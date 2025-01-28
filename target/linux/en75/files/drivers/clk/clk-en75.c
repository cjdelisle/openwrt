// SPDX-License-Identifier: GPL-2.0-only

#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

// Clocks
enum clocks {
	EN75_CLK_BUS,
	EN75_CLK_CPU,
	EN75_CLK_PCIE,
	EN75_CLK_TIMER,
	EN75_NUM_CLOCKS,
};

static const char* clock_names[] = {
	[EN75_CLK_BUS] = "bus",
	[EN75_CLK_CPU] = "cpu",
	[EN75_CLK_PCIE] = "pcie",
	[EN75_CLK_TIMER] = "timer",
};

// Registers
enum en75_register {
	REG_SYSINFO,
	REG_RESET_CONTROL,
	REG_PCI_CONTROL,
	NUM_REGS,
};

#define BANK0 0
#define BANK1 (1U<<31)
static u32 en75_regs[NUM_REGS] = {
	[REG_SYSINFO] = 	BANK1 | 0x284,
	[REG_RESET_CONTROL] = 	BANK1 | 0x88,
	[REG_PCI_CONTROL] = 	BANK1 | 0x834,
};

struct en75_syscon {
	struct regmap *maps[2];
	struct clk_hw pci_clk;
	struct clk_hw_onecell_data *clk_data;
};

static struct en75_syscon en75_syscon;


static inline u32 en75_rreg(enum en75_register reg)
{
	const u32 r = en75_regs[reg];
	u32 out = 0;
	BUG_ON(
		regmap_read(
			en75_syscon.maps[r >> 31],
			(r & ~(1U << 31)),
			&out
		)
	);
	return out;
}

static inline void en75_wreg(enum en75_register reg, u32 val, u32 mask)
{
	const u32 r = en75_regs[reg];
	BUG_ON(
		regmap_update_bits(
			en75_syscon.maps[r >> 31],
			(r & ~(1U << 31)),
			mask,
			val
		)
	);
}

#define   BIT_PCI_PERSTOUT		BIT(29)
#define   BIT_PCI_PERSTOUT1		BIT(26)
#define   BIT_PCI_REFCLK_EN1		BIT(22)

#define   BIT_RESET_PCIEHB		BIT(29)
#define   BIT_RESET_PCIE1		BIT(27)
#define   BIT_RESET_PCIE2		BIT(26)

#define   SYSINFO_BUSCLK_MHZ(word) 	(((word) & GENMASK(19, 10)) >> 10)

#define   CLOCKRATE_CPUTIMER_MHZ 	200

// Fixed Clocks

static inline u32 en75_bus_clock_hz(void)
{
	u32 si = en75_rreg(REG_SYSINFO);
	return SYSINFO_BUSCLK_MHZ(si) * 1000 * 1000;
}

static int en75_register_fixed_clock(
	struct en75_syscon *sysc,
	enum clocks id,
	u32 rate
) {
	struct clk_hw *hw = clk_hw_register_fixed_rate(NULL, clock_names[id], NULL, 0, rate);
	if (IS_ERR(hw)) {
		pr_err("Failed to register bus %s: %ld\n", clock_names[id], PTR_ERR(hw));
		return PTR_ERR(hw);
	}
	sysc->clk_data->hws[id] = hw;
	return 0;
}

// PCI

static int en75_pci_is_enabled(struct clk_hw *hw)
{
	return (en75_rreg(REG_PCI_CONTROL) & BIT_PCI_REFCLK_EN1) != 0;
}

static void en75_pci_unprepare(struct clk_hw *hw)
{
	en75_wreg(REG_PCI_CONTROL, 0, BIT_PCI_REFCLK_EN1);
}

static int en75_pci_prepare(struct clk_hw *hw)
{
	u32 mask;

	// Need to pull device low before reset
	mask = BIT_PCI_PERSTOUT1 | BIT_PCI_PERSTOUT;
	en75_wreg(REG_PCI_CONTROL, 0, mask);
	usleep_range(1000, 2000);

	// Enable PCIe port 1
	en75_wreg(REG_PCI_CONTROL, BIT_PCI_REFCLK_EN1, BIT_PCI_REFCLK_EN1);
	usleep_range(1000, 2000);

	// Reset to default
	mask = BIT_RESET_PCIE1 | BIT_RESET_PCIE2 | BIT_RESET_PCIEHB;
	en75_wreg(REG_RESET_CONTROL, 0, mask);
	usleep_range(1000, 2000);
	en75_wreg(REG_RESET_CONTROL, mask, mask);
	msleep(100);
	en75_wreg(REG_RESET_CONTROL, 0, mask);
	usleep_range(5000, 10000);

	// Release device
	mask = BIT_PCI_PERSTOUT1 | BIT_PCI_PERSTOUT;
	en75_wreg(REG_PCI_CONTROL, 0, mask);
	usleep_range(1000, 2000);
	en75_wreg(REG_PCI_CONTROL, mask, mask);
	msleep(250);

	return 0;
}

static const struct clk_ops pcie_gate_ops = {
	.is_enabled = en75_pci_is_enabled,
	.prepare = en75_pci_prepare,
	.unprepare = en75_pci_unprepare,
};
static const struct clk_init_data pcie_init = {
	.name = "pcie",
	.ops = &pcie_gate_ops,
};

static int en75_register_pcie_clk(struct en75_syscon *sysc, struct device_node *node)
{
	sysc->pci_clk.init = &pcie_init;
	en75_pci_unprepare(&sysc->pci_clk);

	const int ret = of_clk_hw_register(node, &sysc->pci_clk);
	if (ret) {
		pr_err("PCI clock: clk_hw_register() -> %d\n", ret);
		return ret;
	}

	sysc->clk_data->hws[EN75_CLK_PCIE] = &sysc->pci_clk;

	return 0;
}

static void en75_register_clocks(struct en75_syscon *sysc, struct device_node *node)
{
	const u32 bus_rate = en75_bus_clock_hz();
	pr_info("%pOFn: Detected bus clock: %u Mhz\n", node, bus_rate / 1000 / 1000);
	en75_register_fixed_clock(sysc, EN75_CLK_BUS, bus_rate);
	en75_register_fixed_clock(sysc, EN75_CLK_CPU, bus_rate * 4);
	en75_register_fixed_clock(sysc, EN75_CLK_TIMER, CLOCKRATE_CPUTIMER_MHZ * 1000 * 1000);
	en75_register_pcie_clk(sysc, node);
}

static void __init en75_clk_init(struct device_node *node)
{
	pr_info("%pOFn: Init\n", node);

	en75_syscon.clk_data = kzalloc(
		struct_size(en75_syscon.clk_data, hws, EN75_NUM_CLOCKS),
		GFP_KERNEL);
	if (!en75_syscon.clk_data) {
		pr_err("%pOFn: Could not allocate clk_data\n", node);
		return;
	}

	struct regmap *scu = syscon_node_to_regmap(node);
	if (IS_ERR(scu)) {
		pr_err("%pOFn: Could not get sysc syscon regmap: %ld\n",
			node, PTR_ERR(scu));
		return;
	}

	struct regmap *chip_scu =
		syscon_regmap_lookup_by_compatible("econet,en751221-chip-scu");
	if (IS_ERR(chip_scu)) {
		pr_err("%pOFn: Could not get chip-scu regmap: %ld\n",
			node, PTR_ERR(chip_scu));
		return;
	}

	en75_syscon.maps[0] = chip_scu;
	en75_syscon.maps[1] = scu;

	en75_register_clocks(&en75_syscon, node);

	int r = of_clk_add_hw_provider(
		node, of_clk_hw_onecell_get, &en75_syscon.clk_data);
	if (r) {
		pr_err("%pOFn: Could not register clock provider: %d\n", node, r);
		return;
	}
}

CLK_OF_DECLARE(en75_clk, "econet,en75-scu", en75_clk_init);
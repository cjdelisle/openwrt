// SPDX-License-Identifier: GPL-2.0-only
/*
 * Setup for the Realtek RTL838X SoC:
 *	Memory, Timer and Serial
 *
 * Copyright (C) 2020 B. Koblitz
 * based on the original BSP by
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 *
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <linux/irqchip.h>

#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/bootinfo.h>
#include <asm/time.h>
#include <asm/prom.h>
#include <asm/smp-ops.h>
#include <asm/cacheflush.h>
#include <asm/traps.h>

#define VECTORSPACING 0x100	/* for EI/VI mode */

static void __init en75_nmi_setup(void)
{
	pr_debug("mips_nmi_setup()\n");
	void *base;
	extern char except_vec_nmi[];

	base = cpu_has_veic ?
		(void *)(ebase + 0x200 + VECTORSPACING * 64) :
		(void *)(ebase + 0x380);

	pr_info("NMI base is %08x\n", (unsigned int)base);

	/*
	 * Fill the NMI_Handler address in a register, which is a R/W register
	 * start.S will read it, then jump to NMI_Handler address
	 */
	*((volatile u32*)0xbfb00244) = (u32)base;

	memcpy(base, except_vec_nmi, 0x80);
	flush_icache_range((unsigned long)base, (unsigned long)base + 0x80);
}

void __init prom_init(void)
{
	// 1. Bring up early printk
#ifdef CONFIG_EARLY_PRINTK_8250
// We're always big endian so we offset the UART base by 3
#define	UART_BASE		(0xBFBF0000 + 0x03)
#define UART_REG_SHIFT		2
	setup_8250_early_printk_port(
		CKSEG1ADDR(UART_BASE),
		UART_REG_SHIFT,
		0
	);
#endif
	pr_info("%s\n", __func__);

	board_nmi_handler_setup = en75_nmi_setup;
}

// 2. Parse the DT and find memory
void __init plat_mem_setup(void)
{
	pr_info("%s\n", __func__);
	void *dtb;

	set_io_port_base(KSEG1);

	dtb = get_fdt();
	if (!dtb)
		panic("no dtb found");

	__dt_setup_arch(dtb);

	early_init_dt_scan_memory();
}

// 3. Overload __weak device_tree_init()
void __init device_tree_init(void)
{
	pr_info("%s\n", __func__);
	unflatten_and_copy_device_tree();

	// Add SMP registration
	mips_cpc_probe();

	if (!register_cps_smp_ops())
		return;
	if (!register_vsmp_smp_ops())
		return;

	register_up_smp_ops();
}

const char *get_system_type(void)
{
	return "Generic-EN75xx";
}

static void plat_time_init_fallback(void)
{
	struct device_node *np;
	u32 freq = 500000000;

	np = of_find_node_by_name(NULL, "cpus");
	if (!np) {
		pr_err("Missing 'cpus' DT node, using default frequency.");
	} else {
		if (of_property_read_u32(np, "frequency", &freq) < 0)
			pr_err("No 'frequency' property in DT, using default.");
		else
			pr_info("CPU frequency from device tree: %dMHz", freq / 1000000);
		of_node_put(np);
	}
	mips_hpt_frequency = freq / 2;
}

// 4. Initialize the IRQ subsystem
void __init arch_init_irq(void)
{
	pr_info("%s\n", __func__);
	irqchip_init();
}

static __init void en75_of_time_init(void)
{
	struct device_node *np;
	struct clk *clk;

	of_clk_init(NULL);

	mips_hpt_frequency = 0;
	np = of_get_cpu_node(0, NULL);
	if (!np) {
		panic("Failed to get CPU node from DT\n");
	}
	clk = of_clk_get(np, 0);
	if (IS_ERR(clk)) {
		pr_info("Failed to get CPU clock from DT %ld\n", PTR_ERR(clk));
		plat_time_init_fallback();
	} else {
		pr_info("CPU frequency: %luMHz", clk_get_rate(clk) / 1000000);
		mips_hpt_frequency = clk_get_rate(clk) / 2;
		clk_put(clk);
	}
}

void __init plat_time_init(void)
{
	pr_info("%s\n", __func__);

	en75_of_time_init();

	timer_probe();
}
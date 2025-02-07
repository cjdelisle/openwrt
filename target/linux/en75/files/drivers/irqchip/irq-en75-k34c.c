// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * Copyright (C) 2025 Caleb James DeLisle <cjd@cjdns.fr>
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>

#include <asm/setup.h>

#define EN75_PARENT_IRQ_COUNT	8

#define EN75_INTC_IRQ_COUNT	40

// TODO should be in the device tree
#define SI_TIMER1_INT		30
#define SI_TIMER_INT		31

#define INTC_NO_ROUTE		0xff
static_assert(EN75_INTC_IRQ_COUNT < INTC_NO_ROUTE);

enum register_names {
	REG_MASK0,
	REG_MASK1,

	NUM_REGS,
};

// Data

static const struct en75_intc {
	const struct irq_chip chip;

	const struct irq_domain_ops domain_ops;

	void (*dispatch_table[EN75_INTC_IRQ_COUNT])(void);
} en75_intc;

static struct {
	void __iomem *membase;
	struct irq_domain *parent_domain;
	struct irq_domain *self_domain;
	u8 interrupt_routes[EN75_INTC_IRQ_COUNT];
	u32 registers[NUM_REGS];
} en75_intc_rai __ro_after_init = {
	// Register default values
	.registers = {
		[REG_MASK0] = 0x04,
		[REG_MASK1] = 0x50,
	}
};

static DEFINE_SPINLOCK(en75_intc_lock);

// Main

static inline void en75_do_IRQ(const irq_hw_number_t hwirq)
{
	const u8 phwirq = en75_intc_rai.interrupt_routes[hwirq];

	if (hwirq != SI_TIMER_INT && hwirq != SI_TIMER1_INT && phwirq == INTC_NO_ROUTE)
		pr_info("Interrupt on %ld\n", hwirq);

	if (phwirq != INTC_NO_ROUTE)
		do_domain_IRQ(en75_intc_rai.parent_domain, phwirq);
	else
		do_domain_IRQ(en75_intc_rai.self_domain, hwirq);
}


#define IRQ_DISPATCH_FUNC(irq_n)  en75_irqd##irq_n
#define MK_IRQ_DISPATCH(irq_n) \
	static void IRQ_DISPATCH_FUNC(irq_n)(void) { en75_do_IRQ(irq_n); }
#define IRQ_DISPATCH_LIST_ITM(irq_n)	IRQ_DISPATCH_FUNC(irq_n),
#define DO_DISPATCH(X) \
	X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) \
	X(8) X(9) X(10) X(11) X(12) X(13) X(14) X(15) \
	X(16) X(17) X(18) X(19) X(20) X(21) X(22) X(23) \
	X(24) X(25) X(26) X(27) X(28) X(29) X(30) X(31) \
	X(32) X(33) X(34) X(35) X(36) X(37) X(38) X(39)
// static const int en75_dispatch_tbl_size = DO_DISPATCH(+ 1 + 0 *);
static_assert(DO_DISPATCH(+ 1 + 0 *) == EN75_INTC_IRQ_COUNT);
DO_DISPATCH(MK_IRQ_DISPATCH)


static inline void en75_wreg(const u32 reg, const u32 val, const u32 mask)
{
	unsigned long flags;
	u32 v;

	spin_lock_irqsave(&en75_intc_lock, flags);

	v = ioread32(en75_intc_rai.membase + en75_intc_rai.registers[reg]);
	v &= ~mask;
	v |= val & mask;
	iowrite32(v, en75_intc_rai.membase + en75_intc_rai.registers[reg]);

	spin_unlock_irqrestore(&en75_intc_lock, flags);
}

static void en75_chmask(const u32 hwirq, const bool unmask)
{
	const u32 reg = (hwirq > 32);
	const u32 mask = BIT(hwirq - (reg ? 33 : 1));
	const u32 bit = (unmask) ? mask : 0;

	en75_wreg(reg, bit, mask);
}

static void en75_intc_mask(struct irq_data *const d)
{
	pr_info("Masking IRQ %ld on CPU %d\n", d->hwirq, smp_processor_id());
	en75_chmask(d->hwirq, false);
}

static void en75_intc_unmask(struct irq_data *const d)
{
	if (d->hwirq == SI_TIMER_INT && smp_processor_id() != 0) {
		pr_info("Ignore request to unmask IRQ %ld on CPU %d\n", d->hwirq, smp_processor_id());
		return;
	} else if (d->hwirq == SI_TIMER1_INT && smp_processor_id() != 1) {
		pr_info("Ignore request to unmask IRQ %ld on CPU %d\n", d->hwirq, smp_processor_id());
		return;
	}
	pr_info("Unmasking IRQ %ld on CPU %d\n", d->hwirq, smp_processor_id());
	en75_chmask(d->hwirq, true);
}

static inline void en75_mask_all(void)
{
	en75_wreg(REG_MASK0, 0, ~0);
	en75_wreg(REG_MASK1, 0, ~0);
}

// static inline void en75_set_affinity(u32 hwirq) {
// 	/* change IRQ binding to VPE0 or VPE1 */
// 	const u32 regnum = (32 - hwirq) / 4;

// 	// irq 1,  2,  3,  4
// 	// bit 4, 12, 20, 28
// 	const u32 offset = ((hwirq - 1) % 4) * 8 + 4;

// 	// tmp = regRead32((CR_INTC_IVSR0 + regnum * 4));
// 	// if (irq_vpe0 >= irq_vpe1)
// 	// 	tmp &= ~(1<<offset2);
// 	// else
// 	// 	tmp |= (1<<offset2);
// 	// regWrite32((CR_INTC_IVSR0 + regnum * 4), tmp);

// }

static void en75_intc_from_parent(struct irq_desc *const desc)
{
	pr_debug("irq-en75: Got interrupt %d from parent\n",
		irq_desc_get_irq(desc));
}

static int en75_intc_map(struct irq_domain *const d, const u32 irq, const irq_hw_number_t hwirq)
{
	int ret;

	if (hwirq >= EN75_INTC_IRQ_COUNT) {
		pr_err("%s: hwirq %lu out of range\n", __func__, hwirq);
		return -EINVAL;
	} else if (en75_intc_rai.interrupt_routes[hwirq] != INTC_NO_ROUTE) {
		pr_err("%s: hwirq %lu is forwarded\n", __func__, hwirq);
		return -EINVAL;
	}
	if (hwirq == SI_TIMER1_INT || hwirq == SI_TIMER_INT) {
		irq_set_chip_and_handler(
			irq, &en75_intc.chip, handle_percpu_devid_irq);
		ret = irq_set_percpu_devid(irq);
		if (ret) {
			pr_warn("%s: Failed irq_set_percpu_devid for %u: %d\n",
				d->name, irq, ret);
		}
	} else {
		irq_set_chip_and_handler(
			irq, &en75_intc.chip, handle_level_irq);
	}
	set_vi_handler(hwirq, en75_intc.dispatch_table[hwirq]);
	return 0;
}

static const struct en75_intc en75_intc = {
	.chip = {
		.name		= "en75-intc",
		.irq_unmask	= en75_intc_unmask,
		.irq_mask	= en75_intc_mask,
		.irq_mask_ack	= en75_intc_mask,
	},
	.domain_ops = {
		.xlate = irq_domain_xlate_onecell,
		.map = en75_intc_map,
	},
	.dispatch_table = {
		DO_DISPATCH(IRQ_DISPATCH_LIST_ITM)
	},
};

// This should never get used, but we keep it here for completeness.
// We don't want irq-mips-cpu to catch the interrupt because it will
// read it as a bitfield which it's not.
asmlinkage void plat_irq_dispatch(void)
{
	const u32 hwirq = (read_c0_cause() & ST0_IM) >> 10;

	pr_info("%s: hit hwirq %d on CPU %d", __func__, hwirq, smp_processor_id());
	if (WARN_ON_ONCE(hwirq >= EN75_INTC_IRQ_COUNT))
		return;

	en75_do_IRQ(hwirq);
}

// Init

static inline int __init en75_get_routed_interrupts(struct device_node *const node)
{
	const char *field = "econet,route-interrupts";
	const int n_routed_interrupts = of_property_count_u8_elems(node, field);
	u8 *routed_interrupts;

	memset(en75_intc_rai.interrupt_routes, INTC_NO_ROUTE, sizeof(en75_intc_rai.interrupt_routes));
	if (n_routed_interrupts <= 0) {
		return 0;
	} else if (n_routed_interrupts % 2) {
		pr_err("%pOF: %s count is odd, ignoring\n", node, field);
		return 0;
	}
	routed_interrupts = kmalloc_array(n_routed_interrupts, sizeof(u8), GFP_KERNEL);
	if (!routed_interrupts)
		return -ENOMEM;
	if (of_property_read_u8_array(node, field,
		routed_interrupts, n_routed_interrupts)) {
		pr_err("%pOF: Failed to read %s\n", node, field);
		kfree(routed_interrupts);
		return -EINVAL;
	}
	for (int i = 0; i < n_routed_interrupts; i += 2) {
		const u8 irq = routed_interrupts[i];
		const u8 dest = routed_interrupts[i + 1];

		if (irq > EN75_INTC_IRQ_COUNT) {
			pr_err("%pOF: %s[%d] irq(%d) out of range\n",
				node, field, i, irq);
			continue;
		}
		if (dest >= EN75_PARENT_IRQ_COUNT) {
			pr_err("%pOF: %s[%d] dest(%d) out of range\n",
				node, field, i + 1, dest);
			continue;
		}
		en75_intc_rai.interrupt_routes[irq] = routed_interrupts[i + 1];
	}
	kfree(routed_interrupts);
	return 0;
}

static inline int __init en75_prepare_forwarded_irq(
	struct irq_domain *const domain,
	const irq_hw_number_t hwirq)
{
	int irq;

	if (en75_intc_rai.interrupt_routes[hwirq] == INTC_NO_ROUTE)
		return 0;

	set_vi_handler(hwirq, en75_intc.dispatch_table[hwirq]);
	en75_chmask(hwirq, true);

	pr_info("%s: Forwarding HWIRQ %lu -> %u, VIRQ %d mapped and disabled\n",
		domain->name, hwirq, en75_intc_rai.interrupt_routes[hwirq], irq);

	return 0;
}

static int __init en75_intc_of_init(
	struct device_node *const node,
	struct device_node *const parent)
{
	int ret;
	int irq;
	struct irq_data *parent_controller;
	struct resource res;

	pr_info("%pOF: Init\n", node);

	if (!of_property_read_u32_array(node, "econet,intc-registers",
		en75_intc_rai.registers, NUM_REGS)) {
		pr_info("%pOF: using econet,intc-registers from devicetree\n", node);
	}

	ret = en75_get_routed_interrupts(node);
	if (ret)
		return ret;

	irq = irq_of_parse_and_map(node, 0);
	if (!irq) {
		pr_err("%pOF: DT: Failed to get IRQ from 'interrupts'\n", node);
		return -EINVAL;
	}

	parent_controller = irq_get_irq_data(irq);
	if (!parent_controller || !parent_controller->domain) {
		pr_err("%pOF: DT: Failed to get 'interrupt-parent'\n", node);
		ret = -EINVAL;
		goto err_dispose_mapping;
	}
	en75_intc_rai.parent_domain = parent_controller->domain;

	if (of_address_to_resource(node, 0, &res)) {
		pr_err("%pOF: DT: Failed to get 'reg'\n", node);
		ret = -EINVAL;
		goto err_dispose_mapping;
	}

	if (!request_mem_region(res.start, resource_size(&res), res.name)) {
		pr_err("%pOF: Failed to request memory\n", node);
		ret = -EBUSY;
		goto err_dispose_mapping;
	}

	en75_intc_rai.membase = ioremap(res.start, resource_size(&res));
	if (!en75_intc_rai.membase) {
		pr_err("%pOF: Failed to remap membase\n", node);
		ret = -ENOMEM;
		goto err_release;
	}

	en75_mask_all();

	en75_intc_rai.self_domain = irq_domain_add_linear(
		node, EN75_INTC_IRQ_COUNT,
		&en75_intc.domain_ops, NULL);
	if (!en75_intc_rai.self_domain) {
		pr_err("%pOF: Failed to add irqdomain\n", node);
		ret = -ENOMEM;
		goto err_unmap;
	}

	// Anything that is forwarded should be unmasked and then we should forbid
	// any usage of it through this controller.
	for (int i = 0; i < ARRAY_SIZE(en75_intc_rai.interrupt_routes); i++) {
		if (en75_prepare_forwarded_irq(en75_intc_rai.self_domain, i))
			pr_err("%pOF: Failed to prepare forwarded IRQ %d\n", node, i);
	}

	irq_set_chained_handler_and_data(
		irq, en75_intc_from_parent, en75_intc_rai.self_domain);

	// TODO: We don't really "own" these registers, we should find a better way
	write_c0_status((read_c0_status() & ~ST0_IM) |
			(STATUSF_IP0 | STATUSF_IP1));

	return 0;

err_unmap:
	iounmap(en75_intc_rai.membase);
err_release:
	release_mem_region(res.start, resource_size(&res));
err_dispose_mapping:
	irq_dispose_mapping(irq);
	return ret;
}

IRQCHIP_DECLARE(en75_irq_34kc, "econet,en7526-intc", en75_intc_of_init);

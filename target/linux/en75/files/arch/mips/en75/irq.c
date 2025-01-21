// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2009 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 John Crispin <john@phrozen.org>
 * Copyright (C) 2025 Caleb James DeLisle <cjd@cjdns.fr>
 */

#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>

#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>

#include "common.h"

static u32 en75_mask_register[2] = { 0, 0 };

static void __iomem *en75_intc_membase;

static inline void en75_wreg(u32 reg, u32 val, u32 mask)
{
	reg &= 1;
	u32 v = ioread32(en75_intc_membase + en75_mask_register[reg]);
	v &= ~mask;
	v |= val & mask;
	iowrite32(v, en75_intc_membase + en75_mask_register[reg]);
}

static inline u32 en75_rreg(u32 reg)
{
	reg &= 1;
	return ioread32(en75_intc_membase + en75_mask_register[reg]);
}

enum chmask {
	CH_MASK = 0,
	CH_UNMASK,
};

static void en75_chmask(struct irq_data *d, enum chmask change)
{
	u32 reg;
	u32 mask;
	u32 bit;

	if (d->hwirq > 32) {
		mask = BIT(d->hwirq - 33);
		reg = 1;
	} else {
		mask = BIT(d->hwirq - 1);
		reg = 0;
	}

	bit = (change == CH_MASK) ? 0 : mask;

	en75_wreg(reg, bit, mask);
}

static void en75_mask(struct irq_data *d)
{
	en75_chmask(d, 1);
}

static void en75_unmask(struct irq_data *d)
{
	en75_chmask(d, 0);
}

static struct irq_chip en75_irq_chip = {
	.name		= "EN75-INTC",
	.irq_unmask	= en75_unmask,
	.irq_mask	= en75_mask,
	.irq_mask_ack	= en75_mask,
};

// unsupported, disable CONFIG_PERF_EVENTS
//int get_c0_perfcount_int(void)

// MAYBE we can return SI_TIMER_INT which is used in hpt.c
// but most likely we'll have to disable CONFIG_CEVT_R4K
// unsigned int get_c0_compare_int(void)
// {
// 	return CP0_LEGACY_COMPARE_IRQ;
// }

asmlinkage void plat_irq_dispatch(void)
{
	unsigned int irq = (read_c0_cause() & ST0_IM) >> 10;

	do_IRQ(irq);
}

static int intc_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	// Software interrupts must be specifically handled as CPU int.
	// The only purpose for them is SMP which we don't support.
	if (irq == SI_SWINT_INT0 || irq == SI_SWINT1_INT0 ||
		irq == SI_SWINT_INT1 || irq == SI_SWINT1_INT1)
	{
		return -EINVAL;
	}

	if (irq == SI_TIMER_INT || irq == SI_TIMER1_INT) {
			irq_set_chip_and_handler(irq, &en75_irq_chip,
						 handle_percpu_devid_irq);
	} else {
			irq_set_chip_and_handler(irq, &en75_irq_chip,
						 handle_level_irq);
	}

	return 0;
}

static const struct irq_domain_ops irq_domain_ops = {
	.xlate = irq_domain_xlate_onecell,
	.map = intc_map,
};

static int __init intc_of_init(struct device_node *node,
				   struct device_node *parent)
{
	struct resource res;
	struct irq_domain *domain;
	int irq;

	if (of_property_read_u32_array(node, "econet,mask-register",
					&mask_register, 2))
		panic("intc: econet,mask-register is required\n");

	if (of_address_to_resource(node, 0, &res))
		panic("Failed to get intc memory range");

	if (!request_mem_region(res.start, resource_size(&res),
				res.name))
		pr_err("Failed to request intc memory");

	en75_intc_membase = ioremap(res.start,
					resource_size(&res));
	if (!en75_intc_membase)
		panic("Failed to remap intc memory");

	// Clear any pending interrupts.
	clear_c0_cause(CAUSEF_IP);

	// Enable MIPS IRQ0 and IRQ1
	write_c0_status((read_c0_status() & ~ST0_IM) |
			(STATUSF_IP0 | STATUSF_IP1));

	domain = irq_domain_add_legacy(node, 64,
			0, 0, &irq_domain_ops, NULL);
	if (!domain)
		panic("Failed to add irqdomain");

	en75_intc_w32(INTC_INT_GLOBAL, INTC_REG_ENABLE);

	return 0;
}

static struct of_device_id __initdata of_irq_ids[] = {
	{ .compatible = "econet,en7516-intc", .data = intc_of_init },
	{},
};

void __init arch_init_irq(void)
{
	of_irq_init(of_irq_ids);
}
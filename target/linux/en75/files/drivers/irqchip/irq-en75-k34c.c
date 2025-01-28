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

#define SI_TIMER1_INT		30
#define SI_TIMER_INT		31

enum register_names {
	MASK0,
	MASK1,
	REG_COUNT,
};

struct en75_intc {
	void __iomem *membase;
	struct irq_domain* parent_domain;
	struct irq_domain* self_domain;
	u8 interrupt_routes[EN75_INTC_IRQ_COUNT];
	struct irq_chip chip;
	void (*dispatch_table[EN75_INTC_IRQ_COUNT])(void);
	const struct irq_domain_ops domain_ops;
	u32 registers[REG_COUNT];
	spinlock_t lock;
};

static struct en75_intc en75_intc;

static inline void en75_do_IRQ(irq_hw_number_t hwirq)
{
	const u8 nirq = en75_intc.interrupt_routes[hwirq];
	if (nirq) {
		pr_info("Oy, forwarded interrupt! %lu -> %d (we are CPU %d)\n",
			hwirq, nirq, smp_processor_id());
		do_domain_IRQ(en75_intc.parent_domain, nirq);
	} else {
		do_domain_IRQ(en75_intc.self_domain, hwirq);
	}
}

// Setup the IRQ vector table
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
static_assert(EN75_INTC_IRQ_COUNT == 40);

DO_DISPATCH(MK_IRQ_DISPATCH)

static void en75_intc_mask(struct irq_data *d);
static void en75_intc_unmask(struct irq_data *d);
static int en75_intc_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hwirq);

static struct en75_intc en75_intc = {
	.chip = {
		.name		= "en75-intc",
		.irq_unmask	= en75_intc_unmask,
		.irq_mask	= en75_intc_mask,
		.irq_mask_ack	= en75_intc_mask,
	},
	.dispatch_table = {
		DO_DISPATCH(IRQ_DISPATCH_LIST_ITM)
	},
	.domain_ops = {
		.xlate = irq_domain_xlate_onecell,
		.map = en75_intc_map,
	},
	.registers = {
		[MASK0] = 0x04,
		[MASK1] = 0x50,
	},
	.lock = __SPIN_LOCK_UNLOCKED(en75_intc.lock),
};

static inline void en75_wreg(u32 reg, u32 val, u32 mask)
{
	unsigned long flags;
	u32 v;

	spin_lock_irqsave(&en75_intc.lock, flags);

	v = ioread32(en75_intc.membase + en75_intc.registers[reg]);
	v &= ~mask;
	v |= val & mask;
	iowrite32(v, en75_intc.membase + en75_intc.registers[reg]);

	spin_unlock_irqrestore(&en75_intc.lock, flags);
}

enum chmask {
	CH_MASK = 0,
	CH_UNMASK,
};

static void en75_chmask(u32 hwirq, enum chmask change)
{
	const u32 reg = (hwirq > 32);
	const u32 mask = BIT(hwirq - (reg ? 33 : 1));
	const u32 bit = (change == CH_MASK) ? 0 : mask;

	pr_info("en75_chmask: %08x %08x %08x\n", reg, bit, mask);

	en75_wreg(reg, bit, mask);
}

static void en75_intc_mask(struct irq_data *d)
{
	en75_chmask(d->hwirq, CH_MASK);
}

static void en75_intc_unmask(struct irq_data *d)
{
	en75_chmask(d->hwirq, CH_UNMASK);
}

static inline void en75_mask_all(void)
{
	en75_wreg(MASK0, 0, ~0);
	en75_wreg(MASK1, 0, ~0);
}

static void en75_intc_from_parent(struct irq_desc *desc)
{
	pr_debug("irq-en75: Got interrupt %d from parent\n",
		irq_desc_get_irq(desc));
}

static int en75_intc_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hwirq)
{
	int ret;

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

// This should never get used, but we keep it here for completeness.
// We don't want irq-mips-cpu to catch the interrupt because it will
// read it as a bitfield which it's not.
asmlinkage void plat_irq_dispatch(void)
{
	const u32 hwirq = (read_c0_cause() & ST0_IM) >> 10;
	pr_info("plat_irq_dispatch: hit hwirq %d on CPU %d", hwirq, smp_processor_id());
	if (WARN_ON_ONCE(hwirq >= EN75_INTC_IRQ_COUNT)) {
		return;
	}
	en75_do_IRQ(hwirq);
}

static inline int en75_get_routed_interrupts(struct device_node *node)
{
	memset(en75_intc.interrupt_routes, 0, sizeof(en75_intc.interrupt_routes));
	const char* field = "econet,route-interrupts";
	const int n_routed_interrupts =
		of_property_count_u8_elems(node, field);
	if (n_routed_interrupts <= 0) {
		return 0;
	} else if (n_routed_interrupts % 2) {
		pr_err("%pOF: econet,route-interrupts count is odd, ignoring\n", node);
		return 0;
	}
	const u8 *routed_interrupts = kmalloc_array(
		n_routed_interrupts, sizeof(u8), GFP_KERNEL);
	if (!routed_interrupts) {
		return -ENOMEM;
	}
	for (int i = 0; i < n_routed_interrupts; i += 2) {
		const u8 irq = routed_interrupts[i];
		if (irq > EN75_INTC_IRQ_COUNT) {
			pr_err("%pOF: %s[%d] irq(%d) out of range\n",
				node, field, i, irq);
			continue;
		}
		const u8 dest = routed_interrupts[i + 1];
		if (dest >= EN75_PARENT_IRQ_COUNT) {
			pr_err("%pOF: %s[%d] dest(%d) out of range\n",
				node, field, i + 1, dest);
			continue;
		}
		en75_intc.interrupt_routes[irq] = routed_interrupts[i + 1];
	}
	kfree(routed_interrupts);
	return 0;
}

static inline int en75_prepare_forwarded_irq(struct irq_domain *domain, irq_hw_number_t hwirq)
{
	if (!en75_intc.interrupt_routes[hwirq]) {
		return 0;
	}

	const int irq = irq_create_mapping(domain, hwirq);
	if (!irq) {
		return -EINVAL;
	}

	irq_set_chip_and_handler(irq, &en75_intc.chip, handle_bad_irq);
	irq_set_status_flags(irq, IRQ_NOAUTOEN | IRQ_NOREQUEST | IRQ_NOTHREAD);
	set_vi_handler(hwirq, en75_intc.dispatch_table[hwirq]);
	en75_chmask(hwirq, CH_UNMASK);

	return 0;
}

static int __init en75_intc_of_init(struct device_node *node,
					struct device_node *parent)
{
	int ret;
	pr_info("%pOF: Init\n", node);

	if (!of_property_read_u32_array(node, "econet,intc-registers",
					en75_intc.registers, REG_COUNT))
	{
		pr_info("%pOF: using econet,intc-registers from devicetree\n", node);
	}

	ret = en75_get_routed_interrupts(node);
	if (ret) {
		return ret;
	}

	const int irq = irq_of_parse_and_map(node, 0);
	if (!irq) {
		pr_err("%pOF: DT: Failed to get IRQ from 'interrupts'\n", node);
		return -EINVAL;
	}

	struct irq_data *const parent_controller = irq_get_irq_data(irq);
	if (!parent_controller || !parent_controller->domain) {
		pr_err("%pOF: DT: Failed to get 'interrupt-parent'\n", node);
		ret = -EINVAL;
		goto err_dispose_mapping;
	}
	en75_intc.parent_domain = parent_controller->domain;

	struct resource res;
	if (of_address_to_resource(node, 0, &res)) {
		pr_err("%pOF: DT: Failed to get 'reg'\n", node);
		ret = -EINVAL;
		goto err_dispose_mapping;
	}

	if (!request_mem_region(res.start, resource_size(&res),
				res.name))
	{
		pr_err("%pOF: Failed to request memory\n", node);
		ret = -EBUSY;
		goto err_dispose_mapping;
	}

	en75_intc.membase = ioremap(res.start, resource_size(&res));
	if (!en75_intc.membase) {
		pr_err("%pOF: Failed to remap membase\n", node);
		ret = -ENOMEM;
		goto err_release;
	}

	en75_mask_all();

	en75_intc.self_domain = irq_domain_add_linear(
		node, EN75_INTC_IRQ_COUNT,
		&en75_intc.domain_ops, NULL);
	if (!en75_intc.self_domain) {
		pr_err("%pOF: Failed to add irqdomain\n", node);
		ret = -ENOMEM;
		goto err_unmap;
	}

	// Anything that is forwarded should be unmasked and then we should forbid
	// any usage of it through this controller.
	for (int i = 0; i < ARRAY_SIZE(en75_intc.interrupt_routes); i++) {
		if (en75_prepare_forwarded_irq(en75_intc.self_domain, i)) {
			pr_err("%pOF: Failed to prepare forwarded IRQ %d\n", node, i);
		}
	}

	irq_set_chained_handler_and_data(
		irq, en75_intc_from_parent, en75_intc.self_domain);

	return 0;

err_unmap:
	iounmap(en75_intc.membase);
err_release:
	release_mem_region(res.start, resource_size(&res));
err_dispose_mapping:
	irq_dispose_mapping(irq);
	return ret;
}

IRQCHIP_DECLARE(en75_irq_34kc, "econet,en7526-intc", en75_intc_of_init);
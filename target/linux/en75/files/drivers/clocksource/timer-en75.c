// SPDX-License-Identifier: GPL-2.0
/*
 * High Performance Timer present on EN75xx MIPS based SoCs.
 *
 * Copyright (C) 2025 by Caleb James DeLisle <cjd@cjdns.fr>
 */

#include <linux/io.h>
// #include <linux/cache.h>
// #include <linux/kernel.h>
// #include <linux/atomic.h>
#include <linux/cpumask.h>
// #include <linux/irqflags.h>
#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/sched_clock.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/cpuhotplug.h>


// TODO: Move this to the clock driver
#define	CPUTMR_CLK_		(200*1000000)

#define EN75_BITS		32
#define EN75_NUM_BLOCKS		DIV_ROUND_UP(NR_CPUS, 2)
#define EN75_MIN_DELTA		0x00001000
#define EN75_MAX_DELTA		GENMASK(EN75_BITS - 2, 0)

static int en75_cevt_set_next_event(unsigned long delta,
				    struct clock_event_device *dev);

static irqreturn_t en75_cevt_interrupt(int irq, void *dev_id);

static DEFINE_PER_CPU(struct clock_event_device, en75_timer_pcpu) = {
	.rating			= 310,
	.features		= CLOCK_EVT_FEAT_ONESHOT |
				  CLOCK_EVT_FEAT_C3STOP |
				  CLOCK_EVT_FEAT_PERCPU,
	.set_next_event		= en75_cevt_set_next_event,
};

struct en75_timer {
	void __iomem *membase[EN75_NUM_BLOCKS];
	u32 freq_hz;
};
static struct en75_timer en75_timer = {
};

// Each memory block has 2 timers, the order of registers is:
// CTL, CMR0, CNT0, CMR1, CNT1
static inline void __iomem *reg_ctl(u32 timer_n)
{
	return en75_timer.membase[timer_n >> 1];
}
static inline void __iomem *reg_compare(u32 timer_n)
{
	return en75_timer.membase[timer_n >> 1] + (timer_n & 1) * 0x08 + 0x04;
}
static inline void __iomem *reg_count(u32 timer_n)
{
	return en75_timer.membase[timer_n >> 1] + (timer_n & 1) * 0x08 + 0x08;
}
static inline u32 ctl_bit_enabled(u32 timer_n)
{
	return 1U << (timer_n & 1);
}
static inline u32 ctl_bit_pending(u32 timer_n)
{
	return 1U << ((timer_n & 1) + 16);
}

static inline bool en75_cevt_is_pending(int cpu_id)
{
	return ioread32(reg_ctl(cpu_id)) & ctl_bit_pending(cpu_id);
}

void prom_putchar(char c);
static void en75_debug_print(const char *s)
{
	while (*s) {
		if (*s == '\n')
			prom_putchar('\r');
		prom_putchar(*s);
		s++;
	}
}

static irqreturn_t en75_cevt_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *dev = this_cpu_ptr(&en75_timer_pcpu);
	const int cpu = smp_processor_id();

	BUG_ON(cpu != cpumask_first(dev->cpumask));

	if (!en75_cevt_is_pending(cpu)) {
		return IRQ_NONE;
	}

	iowrite32(ioread32(reg_count(cpu)), reg_compare(cpu));
	dev->event_handler(dev);
	return IRQ_HANDLED;
}

static void en75_cevt_enable(const int cpu)
{
	u32 reg = ioread32(reg_ctl(cpu));
	reg |= ctl_bit_enabled(cpu);
	pr_info("en75_cevt_enable(%d) = %08x (%08x)\n", cpu, reg, (u32)reg_ctl(cpu));
	iowrite32(reg, reg_ctl(cpu));
}

static int en75_cevt_set_next_event(unsigned long delta,
				    struct clock_event_device *dev)
{
	const int cpu = cpumask_first(dev->cpumask);

	const u32 next = ioread32(reg_count(cpu)) + delta;
	iowrite32(next, reg_compare(cpu));

	if ((s32)(next - ioread32(reg_count(cpu))) < EN75_MIN_DELTA / 2) {
		return -ETIME;
	}

	return 0;
}

static int en75_cevt_init_cpu(const unsigned int cpu)
{
	struct clock_event_device *cd = &per_cpu(en75_timer_pcpu, cpu);
	pr_info("%s: Setting up clockevent for CPU %d\n", cd->name, cpu);

	const int ret = request_percpu_irq(
		cd->irq, en75_cevt_interrupt,
		cd->name, &en75_timer_pcpu);
	if (ret < 0) {
		pr_err("%s: IRQ %d setup failed (%d)\n", cd->name, cd->irq, ret);
		return ret;
	}

	clockevents_config_and_register(
		cd, en75_timer.freq_hz,
		EN75_MIN_DELTA, EN75_MAX_DELTA);
	en75_cevt_enable(cpu);
	enable_percpu_irq(cd->irq, IRQ_TYPE_NONE);

	return 0;
}

static int __init en75_cevt_init(struct device_node *np)
{
	for (int i = 0; i < num_possible_cpus(); i++) {
		const int irq = irq_of_parse_and_map(np, i);
		if (irq <= 0) {
			pr_err("%pOFn: irq_of_parse_and_map failed for number %d", np, i);
			return -EINVAL;
		}
		per_cpu(en75_timer_pcpu, i).irq = irq;
	}

	cpuhp_setup_state(CPUHP_AP_MIPS_GIC_TIMER_STARTING,
			  "clockevents/en75/timer:starting",
			  en75_cevt_init_cpu, NULL);
	return 0;
}

static u64 notrace en75_sched_clock_read(void)
{
	// Always read from clock zero no matter the CPU
	return (u64)ioread32(reg_count(0));
}

static int __init en75_timer_init(struct device_node *np)
{
	int ret;

	pr_info("%pOFn: Init for %d CPU(s)\n", np, num_possible_cpus());
	en75_timer.freq_hz = CPUTMR_CLK_;

	const int num_blocks = DIV_ROUND_UP(num_possible_cpus(), 2);
	for (int i = 0; i < num_blocks; i++) {
		en75_timer.membase[i] = of_iomap(np, i);
		if (!en75_timer.membase[i]) {
			pr_err("%pOFn: failed to map register [%d]\n",
				np, i);
			return -ENXIO;
		}
	}

	for (int i = 0; i < NR_CPUS; i++) {
		struct clock_event_device *dev = &per_cpu(en75_timer_pcpu, i);
		dev->name = np->name;
		dev->cpumask = cpumask_of(i);
		dev->irq = -1;
	}

	// For clocksource purposes, we ALWAYS read clock zero, no matter what our CPU is.
	ret = clocksource_mmio_init(reg_count(0), np->name,
				    en75_timer.freq_hz, 301, EN75_BITS,
				    clocksource_mmio_readl_up);
	if (ret) {
		pr_err("%pOFn: clocksource_mmio_init failed: %d", np, ret);
		return ret;
	}

	ret = en75_cevt_init(np);
	if (ret < 0) {
		return ret;
	}

	sched_clock_register(en75_sched_clock_read, EN75_BITS,
		en75_timer.freq_hz);

	pr_info("%pOFn: using %u.%03u MHz high precision timer\n", np,
		en75_timer.freq_hz / 1000000,
		(en75_timer.freq_hz / 1000) % 1000);

	return 0;
}

TIMER_OF_DECLARE(en75_timer, "econet,timer-en75", en75_timer_init);
/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI RTI (Real Time Interrupt) timer — Zephyr system clock driver.
 *
 * Used on AM263x / AM263Px Cortex-R5F devices, which have no ARM generic
 * timer and no DM timer; the RTI is the system tick source.
 *
 * The RTI provides a free-running up-counter (FRC0) fed through a prescale
 * up-counter (UC0/CPUC0), with four compare registers (COMP0-3). We use:
 *   - FRC0  as the cycle counter (sys_clock_cycle_get_32)
 *   - COMP0 to generate the kernel tick
 *   - UDCP0 (update-compare 0) for automatic periodic reload of COMP0
 *
 * Register layout from the TI MCU+ SDK (drivers/rti/v0/cslr_rti.h).
 * Structure modeled on drivers/timer/ti_dmtimer.c.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#define DT_DRV_COMPAT ti_rti_timer

/* RTI register offsets (bytes) */
#define RTI_GCTRL    0x00U  /* Global control (counter enables) */
#define RTI_COMPCTRL 0x0CU  /* Compare control (counter select per COMP) */
#define RTI_FRC0     0x10U  /* Free running counter 0 */
#define RTI_UC0      0x14U  /* Up counter 0 (prescale) */
#define RTI_CPUC0    0x18U  /* Compare up counter 0 (prescale value) */
#define RTI_COMP0    0x50U  /* Compare 0 */
#define RTI_UDCP0    0x54U  /* Update compare 0 (auto-add on match) */
#define RTI_SETINT   0x80U  /* Set interrupt enable */
#define RTI_CLEARINT 0x84U  /* Clear interrupt enable */
#define RTI_INTFLAG  0x88U  /* Interrupt flag (write 1 to clear) */

#define RTI_GCTRL_CNT0EN BIT(0)
#define RTI_INT0         BIT(0) /* COMP0 interrupt bit (SETINT/INTFLAG) */

#define TIMER_IRQ_NUM   DT_INST_IRQN(0)
#define TIMER_IRQ_PRIO  DT_INST_IRQ(0, priority)
#define TIMER_IRQ_FLAGS DT_INST_IRQ(0, flags)

#define CYC_PER_TICK \
	((uint32_t)(sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_TICKS ((k_ticks_t)(UINT32_MAX / CYC_PER_TICK) - 1)

struct ti_rti_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
};

struct ti_rti_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct k_spinlock lock;
	uint32_t last_cycle;
};

static const struct device *rti_timer_dev;

static inline uint32_t rti_read(const struct device *dev, uint32_t off)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + off);
}

static inline void rti_write(const struct device *dev, uint32_t off, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + off);
}

static void ti_rti_isr(void *param)
{
	ARG_UNUSED(param);

	struct ti_rti_data *data = rti_timer_dev->data;

	/* No pending COMP0 event? */
	if ((rti_read(rti_timer_dev, RTI_INTFLAG) & RTI_INT0) == 0U) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t curr_cycle = rti_read(rti_timer_dev, RTI_FRC0);
	uint32_t delta_cycles = curr_cycle - data->last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	data->last_cycle += delta_ticks * CYC_PER_TICK;

	/* Acknowledge the COMP0 interrupt (write 1 to clear). */
	rti_write(rti_timer_dev, RTI_INTFLAG, RTI_INT0);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * Periodic mode: COMP0 auto-advances by UDCP0 in hardware,
		 * nothing to reprogram here.
		 */
	}

	k_spin_unlock(&data->lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	struct ti_rti_data *data = rti_timer_dev->data;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks, 1, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t curr_cycle = rti_read(rti_timer_dev, RTI_FRC0);
	uint32_t next_cycle = curr_cycle + (uint32_t)ticks * CYC_PER_TICK;

	/* In tickless mode drive COMP0 directly (no auto-reload step). */
	rti_write(rti_timer_dev, RTI_COMP0, next_cycle);

	k_spin_unlock(&data->lock, key);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return rti_read(rti_timer_dev, RTI_FRC0);
}

unsigned int sys_clock_elapsed(void)
{
	struct ti_rti_data *data = rti_timer_dev->data;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t curr_cycle = rti_read(rti_timer_dev, RTI_FRC0);
	uint32_t delta_ticks = (curr_cycle - data->last_cycle) / CYC_PER_TICK;

	k_spin_unlock(&data->lock, key);

	return delta_ticks;
}

static int sys_clock_driver_init(void)
{
	struct ti_rti_data *data;

	rti_timer_dev = DEVICE_DT_GET(DT_DRV_INST(0));
	data = rti_timer_dev->data;
	data->last_cycle = 0;

	DEVICE_MMIO_NAMED_MAP(rti_timer_dev, reg_base, K_MEM_CACHE_NONE);

	IRQ_CONNECT(TIMER_IRQ_NUM, TIMER_IRQ_PRIO, ti_rti_isr, NULL, TIMER_IRQ_FLAGS);

	/* Stop counter 0 while configuring. */
	rti_write(rti_timer_dev, RTI_GCTRL,
		  rti_read(rti_timer_dev, RTI_GCTRL) & ~RTI_GCTRL_CNT0EN);

	/*
	 * CPUC0 = 1: UC0 matches its compare every RTI input clock, so FRC0
	 * advances at the full RTI_FCLK rate (200 MHz).
	 *
	 * CPUC0 = 0 does NOT mean "divide by 1": UC0 counts up until it equals
	 * CPUC0, and only then is FRC0 incremented and UC0 reset (TRM SPRUJ55
	 * §13.5.1.4.3). With CPUC0 = 0 the match never happens until UC0 wraps
	 * the full 32 bits, i.e. FRC0 ticks once per 2^32 clocks (~21 s) — which
	 * is why the kernel tick previously never fired.
	 */
	rti_write(rti_timer_dev, RTI_CPUC0, 1);
	rti_write(rti_timer_dev, RTI_UC0, 0);
	rti_write(rti_timer_dev, RTI_FRC0, 0);

	/* COMP0 driven by counter 0 (reset default; set explicitly). */
	rti_write(rti_timer_dev, RTI_COMPCTRL, 0);

	/* First tick and periodic auto-reload step. */
	rti_write(rti_timer_dev, RTI_COMP0, CYC_PER_TICK);
	rti_write(rti_timer_dev, RTI_UDCP0,
		  IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? 0 : CYC_PER_TICK);

	/* Clear and enable the COMP0 interrupt. */
	rti_write(rti_timer_dev, RTI_INTFLAG, RTI_INT0);
	rti_write(rti_timer_dev, RTI_SETINT, RTI_INT0);

	irq_enable(TIMER_IRQ_NUM);

	/* Start counter 0. */
	rti_write(rti_timer_dev, RTI_GCTRL,
		  rti_read(rti_timer_dev, RTI_GCTRL) | RTI_GCTRL_CNT0EN);

	return 0;
}

#define TI_RTI_TIMER(n)                                                                  \
	static struct ti_rti_data ti_rti_data_##n;                                       \
	static const struct ti_rti_config ti_rti_config_##n = {                          \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),                    \
	};                                                                               \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &ti_rti_data_##n, &ti_rti_config_##n,        \
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TI_RTI_TIMER);

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

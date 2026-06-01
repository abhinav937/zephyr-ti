/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI AM263x (AM263Px) Cortex-R5F SoC support.
 *
 * Unlike the TI K3 family (AM64x/AM243x), the AM263x has no DMSC/system
 * firmware to negotiate with — the R5F is a primary boot core and there are
 * no control partitions to unlock. Early init therefore only needs to bring
 * up the caches; the MPU is configured by the arch layer from mpu_config.
 */

#include <stdint.h>
#include <zephyr/fatal.h>
#include <zephyr/cache.h>

#include "soc.h"

/* ---- VIM interrupt controller glue (ARM_CUSTOM_INTERRUPT_CONTROLLER) ---- */

unsigned int z_soc_irq_get_active(void)
{
	return z_vim_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	z_vim_irq_eoi(irq);
}

void z_soc_irq_init(void)
{
	z_vim_irq_init();
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	z_vim_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
	z_vim_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	z_vim_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return z_vim_irq_is_enabled(irq);
}

/* Forward declaration for the RTI clock helper below */
static void am263p_enable_rti_clock(void);

void soc_early_init_hook(void)
{
	sys_cache_data_enable();
	sys_cache_instr_enable();

	/*
	 * No K3-style control-partition unlock here — AM263x has no DMSC.
	 * Clock/PLL: the boot ROM / SBL leaves the R5F running at a usable
	 * frequency. Explicit PLL programming via TOP_RCM/MSS_RCM can be added
	 * later if a specific SYSCLK is required (see docs/am263p-memory-map.md).
	 */

	/*
	 * Route and ungate the RTI0 input clock. On the ControlCARD the GEL
	 * script already does this, but doing it here makes the RTI system
	 * timer self-sufficient on a GEL-less boot (SBL/ROM only).
	 */
	am263p_enable_rti_clock();
}

/*
 * Minimal RTI0 clock enable, modeled after TI MCU+ SDK soc_rcm.c.
 * Register addresses from SDK cslr_mss_rcm.h + cslr_soc_baseaddress.h.
 */
static void am263p_enable_rti_clock(void)
{
#define MSS_RCM_BASE             0x53208000U
#define MSS_RCM_RTI0_CLK_SRC_SEL (MSS_RCM_BASE + 0x114U)
#define MSS_RCM_RTI0_CLK_DIV_VAL (MSS_RCM_BASE + 0x214U)
#define MSS_RCM_RTI0_CLK_GATE    (MSS_RCM_BASE + 0x314U)

	/* KICK registers — unlock MSS_RCM before writing clock registers. */
#define MSS_RCM_LOCK0_KICK0      (MSS_RCM_BASE + 0x1008U)
#define MSS_RCM_LOCK0_KICK1      (MSS_RCM_BASE + 0x100CU)

	*(volatile uint32_t *)MSS_RCM_LOCK0_KICK0 = 0x68EF3490U;
	*(volatile uint32_t *)MSS_RCM_LOCK0_KICK1 = 0xD172BC5AU;

	/*
	 * Per upstream mcupsdk-core: SYS_CLK (0x222) is guaranteed running early
	 * (the R5 runs on it); divider 0 = divide-by-1; gate 0 = enabled. Both
	 * SRC_SEL and DIV_VAL use a 3× redundant encoding.
	 */
	*(volatile uint32_t *)MSS_RCM_RTI0_CLK_SRC_SEL = 0x222U; /* SYS_CLK */
	*(volatile uint32_t *)MSS_RCM_RTI0_CLK_DIV_VAL = 0x000U; /* /1 */
	*(volatile uint32_t *)MSS_RCM_RTI0_CLK_GATE    = 0x0U;   /* ungate */
}

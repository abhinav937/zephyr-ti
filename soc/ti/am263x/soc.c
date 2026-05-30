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
}

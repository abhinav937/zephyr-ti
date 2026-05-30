/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x fatal error / exception handler
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_c28x_fatal_error(unsigned int reason,
				      const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		LOG_ERR("C28x Exception:");
		LOG_ERR("  PC    : 0x%08X", esf->pc);
		LOG_ERR("  ST0   : 0x%04X  ST1 : 0x%04X", esf->st0, esf->st1);
		LOG_ERR("  ACC   : 0x%04X%04X (AH:AL)", esf->ah, esf->al);
		LOG_ERR("  P     : 0x%04X%04X (PH:PL)", esf->ph, esf->pl);
		LOG_ERR("  AR0   : 0x%04X  AR1 : 0x%04X", esf->ar0, esf->ar1);
		LOG_ERR("  DP    : 0x%04X  IER : 0x%04X", esf->dp, esf->ier);
		LOG_ERR("  T     : 0x%04X", esf->t);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

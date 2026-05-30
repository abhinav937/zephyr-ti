/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x C runtime initialization — called from reset.S before z_cstart()
 */

#include <zephyr/platform/hooks.h>
#include <zephyr/arch/common/init.h>

/*
 * z_prep_c — called from reset.S after minimal CPU setup.
 *
 * Responsibilities:
 *   1. Run any SoC-level pre-C hooks (clock init, watchdog disable, etc.)
 *   2. Copy .data section from Flash to RAM (if XIP)
 *   3. Hand off to the Zephyr kernel via z_cstart()
 *
 * The BSS clear is handled in reset.S before this function is called,
 * following the pattern used by MIPS and SPARC ports.
 */
FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();

	/* Copy initialized data from Flash to RAM. arch_data_copy() is a no-op
	 * when data is already in RAM (non-XIP builds). */
	arch_data_copy();

	z_cstart();
	CODE_UNREACHABLE;
}

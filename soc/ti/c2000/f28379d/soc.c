/*
 * SPDX-License-Identifier: Apache-2.0
 * TI TMS320F28379D SoC initialization
 */

#include <zephyr/kernel.h>
#include <zephyr/platform/hooks.h>
#include "soc.h"

/*
 * soc_prep_hook — called from z_prep_c() before z_cstart().
 *
 * Runs with interrupts disabled. Safe to access protected registers only
 * after EALLOW. Use this for things that must happen before the kernel
 * scheduler starts: watchdog disable, PLL setup, flash wait states.
 *
 * TODO: Replace inline EALLOW/EDIS with proper asm wrappers once the
 *       toolchain is wired in. For now these are placeholders.
 */
void soc_prep_hook(void)
{
	/*
	 * 1. Disable watchdog — the WDT starts counting after reset and will
	 *    reset the chip if not serviced. Disable it here so the kernel
	 *    can take as long as needed to initialize.
	 *
	 * WDCR is a protected register — requires EALLOW.
	 * The EALLOW/EDIS asm instructions are C28x-specific and cannot be
	 * expressed in portable C; they are provided by the TI CGT intrinsics
	 * or via inline asm.
	 *
	 * TODO: Use __asm(" EALLOW"); / __asm(" EDIS"); (TI CGT syntax)
	 *       or asm volatile ("EALLOW"); (GCC syntax for C28x GCC port)
	 */
	/* __asm(" EALLOW"); */
	/* f28379d_wd_disable(); */
	/* __asm(" EDIS"); */

	/*
	 * 2. Initialize the PLL (SYSPLL) to 200 MHz.
	 *    XTAL = 10 MHz (typical on LAUNCHXL-F28379D)
	 *    IMULT = 40, ODIV = 2 → VCOCLK = 400 MHz → SYSCLK = 200 MHz
	 *
	 * TODO: Port TI DriverLib SysCtl_setClock() or implement directly
	 *       using the SYSPLLCTL1/SYSPLLMULT/SYSCLKDIVSEL registers at
	 *       F28379D_SYSCTRL_BASE.
	 */

	/*
	 * 3. Set Flash wait states for 200 MHz operation.
	 *    At 200 MHz: RWAIT = 3 (see SPRUI33 Table 3-1).
	 *
	 * TODO: Write to Flash->FRDCNTL register (0x5F000C offset) via
	 *       EALLOW-protected write.
	 */

	/*
	 * 4. Enable the PIE controller.
	 *    This is handled in arch_irq_init() called from the kernel.
	 */
}

/*
 * soc_per_core_init_hook — called from arch_kernel_init() for each CPU.
 * For C28x single-core builds this runs once.
 */
void soc_per_core_init_hook(void)
{
	/* Nothing required at kernel init time yet */
}

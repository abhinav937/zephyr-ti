/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI C28x reset vector and startup code.
 *
 * Execution flow after reset:
 *   1. CPU fetches from reset vector (0x3FFFC0 in Flash boot table)
 *   2. Jump to _c28x_reset
 *   3. Initialize stack pointer
 *   4. Set up CPU operating mode (C28x + OBJMODE)
 *   5. Clear BSS
 *   6. Call z_prep_c()
 *
 * Toolchain: TI CGT C2000 (cl2000) or GCC for C28x
 * Reference: TMS320C28x Assembly Language Tools User's Guide (SPRU513)
 */

#include <zephyr/toolchain.h>

/* ------------------------------------------------------------------ */
/* Boot mode — C28x unified memory mode (required for Zephyr)          */
/* ST1 bit 6 = OBJMODE, bit 8 = AMODE (0 = C28x addr mode)            */
/* ------------------------------------------------------------------ */
#define ST1_OBJMODE     (1 << 6)    /* Enable C28x object mode         */

GTEXT(_c28x_reset)
GTEXT(__start)

SECTION_FUNC(reset, _c28x_reset)
__start:
	/* ---- Step 1: Set C28x operating mode ---- */
	/* C28OBJ — enable C28x OBJMODE and clear AMODE */
	C28OBJ                          /* sets ST1.OBJMODE, clears ST1.AMODE */
	C28ADDR                         /* clears ST1.AMODE (C28x addressing) */
	C28MAP                          /* sets M0M1MAP (M0/M1 mapped to prg+data) */
	CLRC    PAGE0                   /* clear PAGE0 (use data space for DP) */
	MOVW    DP, #0                  /* initialize data page pointer */
	CLRC    OVM                     /* disable overflow mode */
	SPM     0                       /* shift product mode 0 */

	/* ---- Step 2: Initialize stack pointer ---- */
	/* _stack_top is defined by the linker script.
	 * SP is word-addressed on C28x, so byte_addr / 2 = word_addr. */
	MOVL    XAR0, #_c28x_stack_top
	MOV     SP, AR0                 /* SP = word address of stack top */

	/* ---- Step 3: Disable all CPU interrupts ---- */
	DINT                            /* global interrupt disable */
	EALLOW                          /* allow protected register access */
	MOVL    XAR0, #0
	MOV     IER, #0                 /* disable all CPU interrupt lines */
	EDIS

	/* ---- Step 4: Clear BSS ---- */
	/* Zero-fill from __bss_start to __bss_end (byte addresses) */
	MOVL    XAR0, #__bss_start
	MOVL    XAR1, #__bss_end
	MOVL    ACC, #0
_bss_clear_loop:
	CMPL    XAR0, XAR1
	BF      _bss_clear_done, GEQ   /* branch if XAR0 >= XAR1 */
	MOVL    *XAR0++, ACC            /* store 32-bit zero, advance pointer */
	SB      _bss_clear_loop, UNC   /* unconditional branch back */
_bss_clear_done:

	/* ---- Step 5: Call C runtime init ---- */
	LCR     z_prep_c                /* long call to z_prep_c() — does not return */

	/* Should never reach here */
_c28x_halt:
	SB      _c28x_halt, UNC

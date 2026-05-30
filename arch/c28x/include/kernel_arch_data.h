/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x DSP kernel architecture data structures
 */

#ifndef ZEPHYR_ARCH_C28X_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_C28X_INCLUDE_KERNEL_ARCH_DATA_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#ifndef _ASMLANGUAGE

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

/*
 * C28x exception (interrupt) stack frame.
 *
 * This is the layout of what the C28x CPU hardware automatically saves to
 * the stack on interrupt entry (14 words = 28 bytes), in stack order
 * (lowest address first after SP has been decremented).
 *
 * Reference: TMS320C28x CPU and Instruction Set Reference Guide (SPRUH18H),
 * Section 1.6 "Interrupt Response".
 */
struct arch_esf {
	uint16_t st0;      /* Status Register 0 */
	uint16_t t;        /* Multiply operand T (high half of XT) */
	uint16_t al;       /* Accumulator low (low half of ACC) */
	uint16_t ah;       /* Accumulator high (high half of ACC) */
	uint16_t pl;       /* Product register low */
	uint16_t ph;       /* Product register high */
	uint16_t ar0;      /* Auxiliary register 0 (low half of XAR0) */
	uint16_t ar1;      /* Auxiliary register 1 (low half of XAR1) */
	uint16_t dp;       /* Data page pointer */
	uint16_t st1;      /* Status Register 1 */
	uint16_t ier;      /* Interrupt Enable Register */
	uint16_t dbgstat;  /* Debug Status Register */
	uint32_t pc;       /* Return PC (22-bit, zero-extended to 32) */
} __packed;

/*
 * C28x thread callee-saved registers.
 *
 * These are saved/restored during a context switch (arch_switch).
 * The hardware auto-saves a subset on interrupt entry (arch_esf above),
 * but for a cooperative context switch we must save everything.
 *
 * Registers NOT in arch_esf that must be saved for context switch:
 *   XAR2-XAR7 (high halves AR2H-AR7H and low halves AR2-AR7)
 *   AR0H, AR1H (high halves of XAR0, XAR1 — not auto-saved)
 *   TL (low half of XT)
 *   RPC (return PC register used by LCR/LRETR)
 *   SP (stack pointer)
 */
struct _callee_saved {
	uint32_t xar0;   /* XAR0 = AR0H:AR0 */
	uint32_t xar1;   /* XAR1 = AR1H:AR1 */
	uint32_t xar2;   /* XAR2 = AR2H:AR2 */
	uint32_t xar3;   /* XAR3 = AR3H:AR3 */
	uint32_t xar4;   /* XAR4 = AR4H:AR4 */
	uint32_t xar5;   /* XAR5 = AR5H:AR5 */
	uint32_t xar6;   /* XAR6 = AR6H:AR6 */
	uint32_t xar7;   /* XAR7 = AR7H:AR7 */
	uint32_t acc;    /* ACC = AH:AL */
	uint32_t p;      /* P = PH:PL */
	uint32_t xt;     /* XT = T:TL */
	uint16_t dp;     /* DP — data page pointer */
	uint16_t st0;    /* ST0 */
	uint16_t st1;    /* ST1 */
	uint16_t ier;    /* IER */
	uint32_t rpc;    /* RPC (22-bit return PC for LCR/LRETR) */
	uint32_t pc;     /* PC — resume address after context switch */
	uint16_t sp;     /* SP — stack pointer (word-addressed, 16-bit) */
	uint16_t _pad;   /* padding for 32-bit struct alignment */
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* nothing extra needed beyond callee_saved for now */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_C28X_INCLUDE_KERNEL_ARCH_DATA_H_ */

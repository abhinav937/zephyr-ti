/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI C28x context switch implementation.
 *
 * z_c28x_context_switch(new_thread, old_thread)
 *   XAR4 = new_thread  (struct k_thread *)
 *   XAR5 = old_thread  (struct k_thread *)
 *
 * Strategy (USE_SWITCH semantics):
 *   1. Save all callee-saved registers into old_thread->callee_saved
 *   2. Save SP into old_thread->callee_saved.sp
 *   3. Save return address (RPC) so old thread resumes after the call
 *   4. Update old_thread->switch_handle to signal the switch is complete
 *   5. Load SP and all callee-saved registers from new_thread->callee_saved
 *   6. Return to new thread's saved PC via LRETR
 *
 * Register layout in struct _callee_saved (see kernel_arch_data.h):
 *   Offset  Size  Field
 *   0x00    4     xar0
 *   0x04    4     xar1
 *   0x08    4     xar2
 *   0x0C    4     xar3
 *   0x10    4     xar4
 *   0x14    4     xar5
 *   0x18    4     xar6
 *   0x1C    4     xar7
 *   0x20    4     acc
 *   0x24    4     p
 *   0x28    4     xt
 *   0x2C    2     dp
 *   0x2E    2     st0
 *   0x30    2     st1
 *   0x32    2     ier
 *   0x34    4     rpc
 *   0x38    4     pc
 *   0x3C    2     sp
 *   0x3E    2     _pad
 *
 * callee_saved is at offset ___callee_saved_t_OFFSET within k_thread.
 * Use the #define from offsets_short_arch.h for portability.
 *
 * Reference: TMS320C28x CPU and Instruction Set Reference Guide (SPRUH18H)
 *            TMS320C28x Optimizing C/C++ Compiler User's Guide (SPRU514)
 */

#include <zephyr/toolchain.h>
#include <offsets_short_arch.h>

/* Offset of callee_saved within struct k_thread */
#define _CALLEE_SAVED_OFF   ___callee_saved_t_OFFSET

GTEXT(z_c28x_context_switch)

/*
 * void z_c28x_context_switch(struct k_thread *new_thread,
 *                             struct k_thread *old_thread)
 * XAR4 = new_thread
 * XAR5 = old_thread
 */
SECTION_FUNC(TEXT, z_c28x_context_switch)

	/* ---- Save old thread context ---- */
	/* XAR6 = &old_thread->callee_saved */
	MOVL    XAR6, XAR5
	ADDB    XAR6, #_CALLEE_SAVED_OFF

	/* Save XAR0-XAR3 (XAR4/XAR5 are args; save below after done using) */
	MOVL    *XAR6, XAR0             /* xar0 @ +0x00 */
	MOVL    *(XAR6+2), XAR1         /* xar1 @ +0x04 (word offset = byte/2) */
	MOVL    *(XAR6+4), XAR2         /* xar2 @ +0x08 */
	MOVL    *(XAR6+6), XAR3         /* xar3 @ +0x0C */
	MOVL    *(XAR6+8), XAR4         /* xar4 @ +0x10 (new_thread, caller-saved but save anyway) */
	MOVL    *(XAR6+10), XAR5        /* xar5 @ +0x14 */
	/* XAR6 and XAR7 */
	MOVL    XAR0, XAR7
	MOVL    *(XAR6+14), XAR0        /* xar7 @ +0x1C */
	/* XAR6 itself — save before we clobber it further */
	/* We need a temporary; use the already-saved XAR6 slot */
	/* XAR6 holds base pointer; save a copy of the original XAR6 */
	/* For now, save 0 as placeholder — TODO: capture original XAR6 */
	MOVL    XAR0, #0
	MOVL    *(XAR6+12), XAR0        /* xar6 @ +0x18 — TODO: save real XAR6 */

	/* Save ACC */
	MOVL    *(XAR6+16), ACC         /* acc @ +0x20 */

	/* Save P */
	MOVL    *(XAR6+18), P           /* p @ +0x24 */

	/* Save XT (T:TL) */
	MOVL    ACC, XT
	MOVL    *(XAR6+20), ACC         /* xt @ +0x28 */

	/* Save DP (16-bit) */
	MOV     AL, DP
	MOV     *(XAR6+22), AL          /* dp @ +0x2C (word offset 22) */

	/* Save ST0 (16-bit) */
	PUSH    ST0                     /* push ST0 to stack */
	POP     AL                      /* pop into AL */
	MOV     *(XAR6+23), AL          /* st0 @ +0x2E */

	/* Save ST1 (16-bit) */
	PUSH    ST1
	POP     AL
	MOV     *(XAR6+24), AL          /* st1 @ +0x30 */

	/* Save IER (16-bit) */
	MOV     AL, IER
	MOV     *(XAR6+25), AL          /* ier @ +0x32 */

	/* Save RPC (return PC for LCR/LRETR) */
	MOVL    ACC, RPC
	MOVL    *(XAR6+26), ACC         /* rpc @ +0x34 */

	/* Save return PC — where old thread resumes after this call returns.
	 * Since this is called via LCR, RPC holds the return address already.
	 * Store RPC as pc so on resume we LRETR to it. */
	MOVL    *(XAR6+28), ACC         /* pc @ +0x38 = same as RPC for LCR caller */

	/* Save SP (16-bit) */
	MOV     AL, SP
	MOV     *(XAR6+30), AL          /* sp @ +0x3C */

	/* Signal switch complete — old_thread->switch_handle = old_thread */
	MOVL    XAR0, XAR5
	MOVL    @_old_thread_switch_handle_ptr, XAR0   /* TODO: use correct offset */

	/* ---- Load new thread context ---- */
	/* XAR6 = &new_thread->callee_saved */
	MOVL    XAR6, XAR4
	ADDB    XAR6, #_CALLEE_SAVED_OFF

	/* Restore SP first so subsequent stack ops are correct */
	MOV     AL, *(XAR6+30)
	MOV     SP, AL

	/* Restore registers */
	MOVL    XAR0, *XAR6             /* xar0 */
	MOVL    XAR1, *(XAR6+2)         /* xar1 */
	MOVL    XAR2, *(XAR6+4)         /* xar2 */
	MOVL    XAR3, *(XAR6+6)         /* xar3 */
	MOVL    XAR4, *(XAR6+8)         /* xar4 */
	MOVL    XAR5, *(XAR6+10)        /* xar5 */
	MOVL    XAR7, *(XAR6+14)        /* xar7 */

	/* Restore ACC */
	MOVL    ACC, *(XAR6+16)

	/* Restore P */
	MOVL    P, *(XAR6+18)

	/* Restore XT */
	MOVL    ACC, *(XAR6+20)
	MOVL    XT, ACC
	MOVL    ACC, *(XAR6+16)         /* restore ACC after XT load */

	/* Restore DP */
	MOV     AL, *(XAR6+22)
	MOV     DP, AL

	/* Restore ST0 / ST1 via push/pop */
	MOV     AL, *(XAR6+23)
	PUSH    AL
	POP     ST0

	MOV     AL, *(XAR6+24)
	PUSH    AL
	POP     ST1

	/* Restore IER */
	MOV     AL, *(XAR6+25)
	MOV     IER, AL

	/* Restore RPC */
	MOVL    ACC, *(XAR6+26)
	MOVL    RPC, ACC

	/* Load resume PC into RPC then LRETR — jumps to new thread's saved PC */
	MOVL    ACC, *(XAR6+28)
	MOVL    RPC, ACC

	/* Restore XAR6 last (it's our base pointer) */
	MOVL    XAR6, *(XAR6+12)        /* xar6 — NOTE: read before XAR6 changes */

	LRETR                           /* long return to RPC = new thread's PC */

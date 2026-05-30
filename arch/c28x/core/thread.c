/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x thread initialization
 */

#include <zephyr/kernel.h>
#include <ksched.h>

/*
 * arch_new_thread — initialize a new thread's register context.
 *
 * Called by k_thread_create(). Sets up the callee_saved struct so that
 * when arch_switch() first resumes this thread, it starts executing at
 * z_thread_entry(entry, p1, p2, p3).
 *
 * C28x calling convention (TI ABI):
 *   Arguments 1-4: XAR4, XAR5, XAR6, XAR7 (32-bit)
 *   Return value:  XAR4 (32-bit) or ACC (16/32-bit)
 *   Callee-saved:  XAR1, XAR2, XAR3 (caller-saved: XAR4-XAR7, ACC, P, T)
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct _callee_saved *ctx = &thread->callee_saved;

	/* Clear everything to a known state */
	(void)memset(ctx, 0, sizeof(*ctx));

	/*
	 * Set up arguments for z_thread_entry(entry, p1, p2, p3).
	 * C28x passes first 4 args in XAR4-XAR7.
	 */
	ctx->xar4 = (uint32_t)(uintptr_t)entry;
	ctx->xar5 = (uint32_t)(uintptr_t)p1;
	ctx->xar6 = (uint32_t)(uintptr_t)p2;
	ctx->xar7 = (uint32_t)(uintptr_t)p3;

	/*
	 * Resume PC: z_thread_entry — the thread starts executing here.
	 * On first context switch in, swap.S will LRETR (long return) to this.
	 */
	ctx->pc = (uint32_t)(uintptr_t)z_thread_entry;

	/*
	 * Stack pointer: stack_ptr is the top of the allocated stack.
	 * C28x SP is word-addressed; byte address / 2 = word address.
	 * The hardware increments SP before a push (post-increment), so
	 * the initial SP should point one word below the first usable slot.
	 *
	 * TODO: Verify with TI CGT ABI — confirm byte vs word addressing
	 *       for the SP initial value with the chosen toolchain.
	 */
	ctx->sp = (uint16_t)((uintptr_t)stack_ptr >> 1);

	/*
	 * ST1: set OBJMODE bit (bit 6) to enable C28x unified memory mode.
	 * Also set AMODE=0 (C28x addressing).
	 * Default value 0x0080 = OBJMODE set, all else clear.
	 */
	ctx->st1 = 0x0080U;

	/* IER: start with all CPU interrupts disabled; kernel enables as needed */
	ctx->ier = 0x0000U;

	/* Required by USE_SWITCH: switch_handle must point to the thread */
	thread->switch_handle = thread;
}

/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x architecture public API header
 *
 * This is included by <zephyr/arch/cpu.h> when CONFIG_C28X=y.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C28X_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_C28X_ARCH_H_

#include <zephyr/arch/c28x/exp.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>

/* Stack alignment: 32-bit (2 words on C28x) */
#define ARCH_STACK_PTR_ALIGN  4U

/*
 * C28x does not support hardware memory protection (no MMU/MPU on C2000).
 * All memory is accessible from any privilege level.
 */
#define ARCH_THREAD_STACK_RESERVED  0U
#define ARCH_KERNEL_STACK_RESERVED  0U

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/irq.h>

/* ---- CPU state access ---- */

/* arch_nop — single C28x no-op instruction */
static ALWAYS_INLINE void arch_nop(void)
{
	__asm volatile("NOP");
}

/* arch_irq_lock — disable global CPU interrupts (DINT), return old key */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;
	/*
	 * On C28x, DINT disables all maskable interrupts by clearing the
	 * INTM bit in ST1. We use ST1 as the "key" for nested lock/unlock.
	 * TODO: Read ST1.INTM into key before DINT.
	 */
	__asm volatile(
		"PUSH  ST1       \n"
		"POP   AL        \n"
		"MOV   %0, AL    \n"
		"DINT            \n"
		: "=r"(key) :: "AL", "memory"
	);
	return key;
}

/* arch_irq_unlock — restore interrupt state from key */
static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	/*
	 * Restore INTM from the saved ST1. If the saved INTM was 0
	 * (interrupts were enabled), re-enable with EINT.
	 * TODO: Proper ST1 restore — push key to stack then pop ST1.
	 */
	if (!(key & BIT(0))) {
		__asm volatile("EINT" ::: "memory");
	}
}

/* arch_irq_unlocked — returns true if key indicates IRQs were enabled */
static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	/* ST1.INTM is bit 0; 0 = enabled, 1 = disabled */
	return !(key & BIT(0));
}

/*
 * ARCH_EXCEPT — trigger a software exception (e.g. for assertions).
 * On C28x, write an illegal value to cause a TRAP, or use a software
 * TRAP instruction if available.
 */
#define ARCH_EXCEPT(reason_p) \
	do { \
		z_c28x_fatal_error(reason_p, NULL); \
	} while (false)

/* Forward declaration — defined in core/fatal.c */
FUNC_NORETURN void z_c28x_fatal_error(unsigned int reason,
				      const struct arch_esf *esf);

/* Arch-specific thread struct additions */
struct _thread_arch {
	/* No extra fields needed for now */
};

typedef struct _thread_arch _thread_arch_t;

/* Arch-specific spinlock type — C28x is single-core, no spinlock HW needed */
struct z_arch_spinlock {
	/* empty on uniprocessor */
};

typedef struct z_arch_spinlock z_arch_spinlock_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_C28X_ARCH_H_ */

/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x kernel architecture function declarations
 */

#ifndef ZEPHYR_ARCH_C28X_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_C28X_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>
#include <zephyr/platform/hooks.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

void z_c28x_fatal_error(unsigned int reason, const struct arch_esf *esf);

void z_c28x_context_switch(struct k_thread *new_thread,
			    struct k_thread *old_thread);

static ALWAYS_INLINE void arch_kernel_init(void)
{
	soc_per_core_init_hook();
}

/*
 * arch_switch — cooperative context switch.
 *
 * switch_to:     next thread's switch_handle (== pointer to the thread struct)
 * switched_from: address of current thread's switch_handle field
 *
 * Uses USE_SWITCH semantics (CONFIG_USE_SWITCH=y).
 */
static inline void arch_switch(void *switch_to, void **switched_from)
{
	struct k_thread *new_thread = switch_to;
	struct k_thread *old_thread =
		CONTAINER_OF(switched_from, struct k_thread, switch_handle);

	z_c28x_context_switch(new_thread, old_thread);
}

static inline bool arch_is_in_isr(void)
{
	return _current_cpu->nested != 0U;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_C28X_INCLUDE_KERNEL_ARCH_FUNC_H_ */

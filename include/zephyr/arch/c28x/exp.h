/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x exception/interrupt stack frame definition (public header)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C28X_EXP_H_
#define ZEPHYR_INCLUDE_ARCH_C28X_EXP_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

/*
 * Exception stack frame — what the C28x CPU hardware saves to the stack
 * on interrupt entry (14 words = 28 bytes).
 *
 * This struct is used as the `esf` parameter in Zephyr's fatal error and
 * IRQ handling APIs (z_fatal_error, LOG_ERR, etc.).
 */
struct arch_esf {
	uint16_t st0;      /* Status Register 0 */
	uint16_t t;        /* Multiply operand T */
	uint16_t al;       /* Accumulator low */
	uint16_t ah;       /* Accumulator high */
	uint16_t pl;       /* Product register low */
	uint16_t ph;       /* Product register high */
	uint16_t ar0;      /* Aux register 0 (XAR0 low half) */
	uint16_t ar1;      /* Aux register 1 (XAR1 low half) */
	uint16_t dp;       /* Data page pointer */
	uint16_t st1;      /* Status Register 1 */
	uint16_t ier;      /* Interrupt Enable Register */
	uint16_t dbgstat;  /* Debug Status Register */
	uint32_t pc;       /* Return PC (22-bit, zero-extended) */
} __packed;

typedef struct arch_esf z_arch_esf_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_C28X_EXP_H_ */

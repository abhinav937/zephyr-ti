/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x short offsets for callee-saved struct — used in assembly.
 */

#ifndef ZEPHYR_ARCH_C28X_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_C28X_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_xar0  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar0_OFFSET)
#define _thread_offset_to_xar1  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar1_OFFSET)
#define _thread_offset_to_xar2  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar2_OFFSET)
#define _thread_offset_to_xar3  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar3_OFFSET)
#define _thread_offset_to_xar4  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar4_OFFSET)
#define _thread_offset_to_xar5  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar5_OFFSET)
#define _thread_offset_to_xar6  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar6_OFFSET)
#define _thread_offset_to_xar7  \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xar7_OFFSET)
#define _thread_offset_to_acc   \
	(___callee_saved_t_OFFSET + ___callee_saved_t_acc_OFFSET)
#define _thread_offset_to_p     \
	(___callee_saved_t_OFFSET + ___callee_saved_t_p_OFFSET)
#define _thread_offset_to_xt    \
	(___callee_saved_t_OFFSET + ___callee_saved_t_xt_OFFSET)
#define _thread_offset_to_dp    \
	(___callee_saved_t_OFFSET + ___callee_saved_t_dp_OFFSET)
#define _thread_offset_to_st0   \
	(___callee_saved_t_OFFSET + ___callee_saved_t_st0_OFFSET)
#define _thread_offset_to_st1   \
	(___callee_saved_t_OFFSET + ___callee_saved_t_st1_OFFSET)
#define _thread_offset_to_ier   \
	(___callee_saved_t_OFFSET + ___callee_saved_t_ier_OFFSET)
#define _thread_offset_to_rpc   \
	(___callee_saved_t_OFFSET + ___callee_saved_t_rpc_OFFSET)
#define _thread_offset_to_pc    \
	(___callee_saved_t_OFFSET + ___callee_saved_t_pc_OFFSET)
#define _thread_offset_to_sp    \
	(___callee_saved_t_OFFSET + ___callee_saved_t_sp_OFFSET)

#endif /* ZEPHYR_ARCH_C28X_INCLUDE_OFFSETS_SHORT_ARCH_H_ */

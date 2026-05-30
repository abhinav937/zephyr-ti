/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * AM263x pinctrl SoC definitions.
 *
 * Each pin is described by an absolute PADCONFIG register address and the
 * value to write (mux mode + pad attributes). This avoids needing a separate
 * pinctrl controller device node.
 */

#ifndef ZEPHYR_SOC_TI_AM263X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_TI_AM263X_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pinctrl_soc_pin {
	uint32_t reg;  /* absolute PADCONFIG register address */
	uint32_t val;  /* value to write (mux mode + pad config) */
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.reg = DT_PROP_BY_IDX(DT_PROP_BY_IDX(node_id, prop, idx), pinmux, 0),               \
		.val = DT_PROP_BY_IDX(DT_PROP_BY_IDX(node_id, prop, idx), pinmux, 1),               \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_TI_AM263X_PINCTRL_SOC_H_ */

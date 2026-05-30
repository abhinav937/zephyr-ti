/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal pinctrl driver for TI AM263x. Each pin carries the absolute
 * PADCONFIG register address and the value to write, so no separate pinctrl
 * controller device is required.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		sys_write32(pins[i].val, (mem_addr_t)pins[i].reg);
	}

	return 0;
}

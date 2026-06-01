/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * AM263P ControlCARD bring-up sample: alternate the two USER LEDs using the
 * Zephyr GPIO API, paced by the RTI system timer.
 *
 * Everything board-specific now lives in devicetree:
 *   - LED pins:  aliases led0/led1 -> gpio-leds (LD6 = gpio0/22, LD7 = gpio1/26)
 *   - Pad mux:   pinctrl-0 on &gpio0/&gpio1 routes the pads to GPIO mode 7
 *   - GPIO bank: "ti,davinci-gpio" on bank01 (gpio0) and bank23 (gpio1)
 * The LEDs are active-LOW; GPIO_ACTIVE_LOW in DT handles the inversion, so the
 * logical level here reads the obvious way (1 = lit).
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

int main(void)
{
	if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1)) {
		printk("LED GPIO controller not ready\n");
		return 0;
	}

	/* Start with both LEDs off (logical inactive). */
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);

	printk("\n*** AM263P ControlCARD: Zephyr console is up ***\n");
	printk("Build time: %s %s\n", __DATE__, __TIME__);
	printk("sys_clock_hw_cycles_per_sec = %u\n", sys_clock_hw_cycles_per_sec());

	uint32_t beat = 0U;

	while (1) {
		/* Alternate: LD6 then LD7. */
		gpio_pin_set_dt(&led0, 1);
		gpio_pin_set_dt(&led1, 0);
		k_msleep(500);

		gpio_pin_set_dt(&led0, 0);
		gpio_pin_set_dt(&led1, 1);
		k_msleep(500);

		printk("uptime %lld ms, beat %u\n", k_uptime_get(), beat++);
	}

	return 0;
}

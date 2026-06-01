/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal AM263P ControlCARD smoke test: alternate LD6/LD7 via the GPIO API,
 * paced by the RTI system timer. No shell — see samples/am263_app for the
 * Phase 5 application skeleton.
 *
 * LEDs are active-LOW; GPIO_ACTIVE_LOW in DT handles the inversion (1 = lit).
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

	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);

	printk("\n*** AM263P ControlCARD: Zephyr console is up ***\n");
	printk("Build time: %s %s\n", __DATE__, __TIME__);
	printk("sys_clock_hw_cycles_per_sec = %u\n", sys_clock_hw_cycles_per_sec());

	uint32_t beat = 0U;

	while (1) {
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

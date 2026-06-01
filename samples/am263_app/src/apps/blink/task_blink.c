/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "apps/blink/task_blink.h"

#define BLINK_STACK_SIZE 1024
#define BLINK_PRIORITY   7

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static K_THREAD_STACK_DEFINE(blink_stack, BLINK_STACK_SIZE);
static struct k_thread blink_thread;

static volatile bool blink_enabled = true;
static volatile uint32_t blink_interval_ms = 500U; /* on-time per LED */

static void blink_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		if (blink_enabled) {
			gpio_pin_set_dt(&led0, 1);
			gpio_pin_set_dt(&led1, 0);
			k_msleep(blink_interval_ms);

			gpio_pin_set_dt(&led0, 0);
			gpio_pin_set_dt(&led1, 1);
			k_msleep(blink_interval_ms);
		} else {
			gpio_pin_set_dt(&led0, 0);
			gpio_pin_set_dt(&led1, 0);
			k_msleep(100);
		}
	}
}

void task_blink_start(void)
{
	if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1)) {
		printk("task_blink: LED GPIO controller not ready\n");
		return;
	}

	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);

	k_thread_create(&blink_thread, blink_stack, K_THREAD_STACK_SIZEOF(blink_stack),
			blink_entry, NULL, NULL, NULL, BLINK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&blink_thread, "task_blink");
}

void task_blink_set_enabled(bool enabled)
{
	blink_enabled = enabled;
}

bool task_blink_get_enabled(void)
{
	return blink_enabled;
}

void task_blink_set_interval(uint32_t ms)
{
	if (ms > 0U) {
		blink_interval_ms = ms;
	}
}

uint32_t task_blink_get_interval(void)
{
	return blink_interval_ms;
}

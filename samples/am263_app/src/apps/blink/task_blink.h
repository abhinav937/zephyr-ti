/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * task_blink — a background task (thread) that alternates LD6/LD7. Exposes a
 * small control API used by cmd_blink (the shell front-end).
 */

#ifndef AM263_APP_TASK_BLINK_H
#define AM263_APP_TASK_BLINK_H

#include <stdbool.h>
#include <stdint.h>

/* Configure the LEDs and start the blink thread. */
void task_blink_start(void);

/* Enable/disable blinking. When disabled, both LEDs are held off. */
void task_blink_set_enabled(bool enabled);
bool task_blink_get_enabled(void);

/* Per-LED on-time in ms: each LED stays lit this long, then they swap. Must be > 0. */
void task_blink_set_interval(uint32_t ms);
uint32_t task_blink_get_interval(void);

#endif /* AM263_APP_TASK_BLINK_H */

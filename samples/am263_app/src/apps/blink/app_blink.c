/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "apps/blink/app_blink.h"
#include "apps/blink/task_blink.h"

void app_blink_init(void)
{
	task_blink_start();
	/* cmd_blink registers its shell group at link time (SHELL_CMD_REGISTER). */
}

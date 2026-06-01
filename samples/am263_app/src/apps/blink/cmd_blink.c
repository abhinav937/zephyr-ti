/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * cmd_blink — the `blink` shell command group, front-end to task_blink.
 */

#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include "apps/blink/task_blink.h"

static int cmd_blink_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	task_blink_set_enabled(true);
	shell_print(sh, "blink on");
	return 0;
}

static int cmd_blink_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	task_blink_set_enabled(false);
	shell_print(sh, "blink off");
	return 0;
}

static int cmd_blink_rate(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int ms = atoi(argv[1]);

	if (ms <= 0) {
		shell_error(sh, "rate must be > 0 ms");
		return -EINVAL;
	}

	task_blink_set_period((uint32_t)ms);
	shell_print(sh, "blink rate %d ms", ms);
	return 0;
}

static int cmd_blink_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "blink: %s, %u ms",
		    task_blink_get_enabled() ? "on" : "off",
		    task_blink_get_period());
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(blink_cmds,
	SHELL_CMD(on, NULL, "Enable blinking.", cmd_blink_on),
	SHELL_CMD(off, NULL, "Disable blinking (LEDs off).", cmd_blink_off),
	SHELL_CMD_ARG(rate, NULL, "Set half-period: rate <ms>.", cmd_blink_rate, 2, 0),
	SHELL_CMD(status, NULL, "Show blink state.", cmd_blink_status),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(blink, &blink_cmds, "LED blink task control", NULL);

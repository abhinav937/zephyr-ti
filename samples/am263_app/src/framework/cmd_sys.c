/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Framework-level shell commands (always present): the `am263` group.
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

static int cmd_uptime(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "uptime: %lld ms", k_uptime_get());
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(am263_cmds,
	SHELL_CMD(uptime, NULL, "Print system uptime (ms).", cmd_uptime),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(am263, &am263_cmds, "AM263P platform commands", NULL);

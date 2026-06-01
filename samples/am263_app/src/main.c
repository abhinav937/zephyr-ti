/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * AM263P application skeleton (Phase 5).
 *
 * main() only brings up the platform: it starts the Kconfig-enabled apps and
 * returns. Everything interactive runs in the shell thread; periodic work runs
 * in each app's task thread. This is the in-tree seed of the AMDC-style
 * app_/task_/cmd_ layout, to be extracted to the out-of-tree Layer-2 workspace.
 */

#include <zephyr/kernel.h>

#include "framework/app.h"

int main(void)
{
	printk("\n*** AM263P application platform up (Phase 5) ***\n");
	printk("Build time: %s %s\n", __DATE__, __TIME__);
	printk("Try: 'help', 'am263 uptime', 'blink status', 'blink interval 1000'\n");

	apps_init();
	return 0;
}

/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tiny app-registration layer (Layer-2 pattern, in-tree for now).
 *
 * Each application provides an `app_<name>_init()` that starts its task(s) and
 * (via SHELL_CMD_REGISTER) contributes its `cmd_` group. apps_init() calls the
 * init of every app enabled through Kconfig. This is the seed of the AMDC-style
 * app_/task_/cmd_ structure; it will be extracted to the out-of-tree Layer-2
 * workspace once the pattern settles.
 */

#ifndef AM263_FRAMEWORK_APP_H
#define AM263_FRAMEWORK_APP_H

/* Initialise all Kconfig-enabled applications. Call once from main(). */
void apps_init(void);

#endif /* AM263_FRAMEWORK_APP_H */

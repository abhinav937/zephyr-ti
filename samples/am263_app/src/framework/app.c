/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "framework/app.h"

#if defined(CONFIG_AM263_APP_BLINK)
#include "apps/blink/app_blink.h"
#endif

void apps_init(void)
{
#if defined(CONFIG_AM263_APP_BLINK)
	app_blink_init();
#endif
}

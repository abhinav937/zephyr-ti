/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * AM263x Cortex-R5F linker script. Memory regions come from the board
 * devicetree (zephyr,sram chosen node -> OCRAM). The Cortex-A/R common
 * linker script lays out .text/.data/.bss into the SRAM region.
 */

#include <zephyr/arch/arm/cortex_a_r/scripts/linker.ld>

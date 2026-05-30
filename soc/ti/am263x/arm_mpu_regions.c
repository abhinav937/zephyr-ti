/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM MPU region configuration for the AM263x Cortex-R5F.
 */

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#ifdef CONFIG_USERSPACE
#define PERM_Msk P_RW_U_RO_Msk
#else
#define PERM_Msk P_RW_U_NA_Msk
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/*
	 * Device/peripheral space (UART, VIM, GPIO, EPWM, ADC, RCM …) lives in
	 * 0x4000_0000-0x5FFF_FFFF on the AM263x. Map the low 2 GB as device,
	 * non-executable; the SRAM region below overrides its window with
	 * cacheable normal memory.
	 */
	MPU_REGION_ENTRY("Device", 0x0, REGION_2G,
			 {MPU_RASR_S_Msk | NOT_EXEC | PERM_Msk}),

	/* Main RAM (OCRAM) — cacheable normal memory, executable. */
	MPU_REGION_ENTRY(
		"SRAM", DT_CHOSEN_SRAM_ADDR, REGION_SRAM_SIZE,
		{NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE | PERM_Msk}),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};

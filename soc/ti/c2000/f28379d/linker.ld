/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI TMS320F28379D linker script for Zephyr RTOS (CPU1)
 *
 * Memory map (C28x word addresses, 16-bit words):
 *   M0 RAM:    0x000000 - 0x0003FF  (1KB)
 *   M1 RAM:    0x000400 - 0x0007FF  (1KB)
 *   LS0-LS5:   0x008000 - 0x00AFFF  (12KB, CPU1-owned)
 *   GS0-GS7:   0x00C000 - 0x00DFFF  (8KB, CPU1 default)
 *   Flash:     0x080000 - 0x09FFFF  (128K words = 256KB, CPU1 Bank 0)
 *
 * NOTE: C28x addresses are in 16-bit word units. The byte address seen
 * by the toolchain may be word_address * 2 depending on the C model
 * (C28x unified memory mode = byte-addressable in C code, but the
 * hardware program counter increments in words).
 *
 * The addresses below use the C28x unified (byte) address space as seen
 * by the TI CGT C compiler in C28x mode (LARGE_MODEL / UNIFIED_MEMORY).
 *
 * Reference: SPRUI33 Section 1.4 (Memory Map)
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* Device Tree provides sram/flash sizes via DTS chosen nodes */
#define FLASH_ADDR  DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define FLASH_SIZE  DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#define RAM_ADDR    DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE    DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

MEMORY
{
	/* Flash: code and read-only data */
	FLASH (rx) : ORIGIN = FLASH_ADDR, LENGTH = FLASH_SIZE

	/* RAM: stack, BSS, data */
	RAM (rwx)  : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
}

/* Zephyr standard region aliases */
REGION_ALIAS("REGION_TEXT",       FLASH);
REGION_ALIAS("REGION_RODATA",     FLASH);
REGION_ALIAS("REGION_RESET",      FLASH);
REGION_ALIAS("REGION_DATA_VMA",   RAM);
REGION_ALIAS("REGION_DATA_LMA",   FLASH);  /* LMA in Flash for XIP copy */
REGION_ALIAS("REGION_BSS",        RAM);
REGION_ALIAS("REGION_NOINIT",     RAM);

/* Tell Zephyr linker macros which regions to use */
#define ROMABLE_REGION  FLASH
#define RAMABLE_REGION  RAM

/* Include Zephyr common linker definitions */
#include <zephyr/linker/sections.ld>

/* C28x boot vector — must be at 0x3FFFC0 (word address) in Flash.
 * The boot ROM jumps here on power-up.
 * TODO: Place reset section at the correct Flash sector start. */
SECTIONS
{
	.reset_vector :
	{
		KEEP(*(.reset))         /* reset.S SECTION_FUNC(reset, ...) */
	} > FLASH

	/* PIE Vector Table — must be at 0x000D00 (word addr = 0x001A00 byte) */
	.pie_vect (NOLOAD) :
	{
		. = ALIGN(256);         /* 256-word (512-byte) aligned */
		KEEP(*(.pie_vect))
	} > RAM

	/* Stack — placed at top of RAM */
	.stack (NOLOAD) :
	{
		. = ALIGN(2);
		_c28x_stack_top = . + CONFIG_MAIN_STACK_SIZE;
		*(.stack)
	} > RAM
}

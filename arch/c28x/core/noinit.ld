/*
 * SPDX-License-Identifier: Apache-2.0
 * Linker fragment for Zephyr noinit section on TI C28x
 */

SECTION_PROLOGUE(.z_noinit, (NOLOAD),)
{
	*(.z_noinit)
	*(".z_noinit.*")
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

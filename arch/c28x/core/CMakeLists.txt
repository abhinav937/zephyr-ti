# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  fatal.c
  prep_c.c
  thread.c
  irq_manage.c
  reset.S
  swap.S
)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)

zephyr_linker_sources(NOINIT noinit.ld)

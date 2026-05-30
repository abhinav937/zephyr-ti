# SPDX-License-Identifier: Apache-2.0

# TI C28x DSP architecture support for Zephyr RTOS
# Target: TMS320C28x family (C2000 microcontrollers, e.g. F28379D)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-c28x")

zephyr_include_directories(include)

add_subdirectory(core)

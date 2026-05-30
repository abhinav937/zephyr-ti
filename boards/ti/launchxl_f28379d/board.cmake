# SPDX-License-Identifier: Apache-2.0
#
# Board flash/debug configuration for LAUNCHXL-F28379D
#
# The LaunchPad uses the TI XDS110 debug probe (built-in).
# Programming uses OpenOCD or TI Uniflash / CCS.
#
# TODO: Add OpenOCD config when C2000 target is supported upstream.
#       TI XDS110 requires the TI JTAG transport (not generic ARM SWD).
#       Alternatively, use TI's DSLite (part of CCS) for flashing.

# Prefer dsLite (CCS command-line tool) if available
find_program(DSLITE dsLite PATH_SUFFIXES bin)

if(DSLITE)
  set(BOARD_DEBUG_RUNNER dslite)
  board_runner_args(dslite "--config=${CMAKE_CURRENT_LIST_DIR}/xds110.ccxml")
  include(${ZEPHYR_BASE}/boards/common/dslite.board.cmake OPTIONAL)
else()
  message(WARNING
    "dsLite not found. Install Code Composer Studio and add it to PATH, "
    "or use TI Uniflash to flash ${CMAKE_BINARY_DIR}/zephyr/zephyr.out")
endif()

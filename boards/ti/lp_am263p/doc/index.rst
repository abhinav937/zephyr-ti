.. _lp_am263p:

TI LP-AM263P LaunchPad
######################

Overview
********

Experimental Zephyr port for the TI Sitara **AM263P4** (quad Arm Cortex-R5F
@ 400 MHz) on the **LP-AM263P** LaunchPad. This is a work-in-progress bring-up:
``hello_world`` and ``synchronization`` build and link; runtime has not yet been
confirmed on silicon (see `Status and verify-list`_).

Supported for now:

* Primary core: **R5FSS0 Core 0** (board target ``lp_am263p/am263p4/r5f0_0``)
* UART0 console (XDS110 USB-UART), 115200 8N1
* RTI0 system tick (custom ``ti,rti-timer`` driver)
* VIM interrupt controller (reused in-tree driver)

Building
********

Set up a standard Zephyr environment (this repo is the manifest/zephyr tree).

Windows (PowerShell), one-time
==============================

.. code-block:: powershell

   # Prereqs: install Python 3.12+, Git, and Zephyr's build deps per
   # https://docs.zephyrproject.org/latest/develop/getting_started/index.html
   #   (choco install cmake ninja gperf dtc-msys2 wget 7zip)

   # Clone your fork, then create a venv + west workspace around it:
   git clone https://github.com/abhinav937/zephyr-ti.git
   python -m venv .venv-zephyr
   .\.venv-zephyr\Scripts\activate
   pip install west
   west init -l zephyr-ti
   west update --narrow -o=--depth=1 cmsis
   pip install -r zephyr-ti\scripts\requirements.txt

   # Install the ARM toolchain from the Zephyr SDK:
   west sdk install -t arm-zephyr-eabi

Build
=====

.. code-block:: powershell

   .\.venv-zephyr\Scripts\activate
   $env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
   west build -b lp_am263p/am263p4/r5f0_0 zephyr-ti\samples\hello_world -p always

Output: ``build\zephyr\zephyr.elf`` (links into OCRAM at ``0x70040000``).

Flashing / running (first bring-up)
***********************************

The LP-AM263P has an onboard **XDS110** debug probe. For initial bring-up the
simplest path is to load the ELF directly to the R5F via **Code Composer
Studio** (CCS) — no SBL/flash image needed, because CCS sets the PC to the
ELF entry point after loading:

1. Open CCS, create a target configuration for **AM263Px** via **XDS110**.
2. Launch the target; connect to **R5FSS0_0** (Core 0).
3. *Run > Load > Load Program* → select ``build\zephyr\zephyr.elf``.
4. Open a serial terminal on the XDS110 UART COM port @ **115200 8N1**.
5. Run. You should see ``Hello World! lp_am263p/am263p4/r5f0_0``.

(Standalone boot from flash via the SBL is a later step; the debugger-load
path above is the fastest way to validate the port.)

.. _Status and verify-list:

Status and verify-list (confirm on hardware)
********************************************

If something doesn't work on first run, check these — they are the values that
could not be verified without silicon:

#. **No UART output at all** → UART0 pin mux. There is no pinctrl config yet;
   the port relies on the ROM/SBL having muxed the console pins. Confirm the
   LP-AM263P boot mode leaves UART0 muxed, or add a pinctrl-0 to ``&uart0``.
#. **Garbled UART output** → baud divisor. ``uart0`` ``clock-frequency`` is set
   to 96 MHz (``dts/arm/ti/am263p4.dtsi``); correct it to the real AM263Px UART
   functional clock if characters are wrong.
#. **Wrong timing** (``k_sleep`` too fast/slow) → RTI input clock.
   ``CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC`` defaults to 25 MHz
   (``soc/ti/am263x/Kconfig.defconfig``); set it to the actual RTI counter
   clock from the RCM.
#. **Hang / no ticks** → RTI prescale. ``ti_rti_timer.c`` assumes
   ``RTICPUC0 = 0`` gives a divide-by-1 (FRC0 at input clock); verify against
   the TRM (SPRUJ55).

See ``docs/PLAN-am263p.md`` and ``docs/am263p-memory-map.md`` for the full
porting notes and the verified register/address sources.

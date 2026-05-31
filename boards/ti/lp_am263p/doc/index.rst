.. _lp_am263p:

TI LP-AM263P LaunchPad
######################

.. attention::
   **Experimental — First Silicon Bring-up**

   This is a brand-new Zephyr port (May 2026). ``hello_world`` builds and links
   cleanly, but **has never been executed on real AM263P4 silicon**. You are
   about to perform the first on-hardware validation.

   Expect to spend time on clock frequencies, RTI tick rate, and UART pinmux
   confirmation. The verify-list below captures the exact unknowns.

Overview
********

Zephyr support for the **TI LP-AM263P LaunchPad** (AM263P4 quad Cortex-R5F
@ 400 MHz). Target: ``lp_am263p/am263p4/r5f0_0`` (R5FSS0 Core 0).

**Note for ControlCARD users (TMDSCNCD263P):** The current board files target
the LaunchPad. On the ControlCARD you will see different measured clocks from
the gel (UART ~160 MHz, RTI ~200 MHz) and possibly different UART pin routing.

Current status:

* Builds and links: ``hello_world``, ``synchronization``
* Console: UART0 via XDS110 (115200 8N1)
* System tick: Custom RTI driver (``ti_rti_timer``)
* Interrupt controller: Reused VIM driver
* No pinctrl driver in use yet (relies on ROM/SBL mux for UART0)
* OpenOCD support is a placeholder — use CCS for first bring-up

Windows End-to-End Setup & Test Guide
*************************************

This section walks you through a complete Windows setup so you can plug in the
LP-AM263P and run Zephyr today.

Prerequisites (one-time)
========================

.. list-table::
   :header-rows: 1

   * - Tool
     - Recommended
     - Why
   * - **Windows 10/11** (64-bit)
     - Latest
     - PowerShell + long path support
   * - **Git for Windows**
     - 2.45+
     - **Critical**: During install choose *"Checkout as-is, commit as-is"* (or "as-is / Unix-style line endings")
   * - **Python**
     - 3.12 or 3.13 from python.org
     - Add to PATH during install. Do **not** use Microsoft Store Python.
   * - **Code Composer Studio (CCS)**
     - 12.8 or newer (free with TI account)
     - Required for reliable XDS110 debug on AM263Px
   * - **Build tools**
     - cmake, ninja, dtc
     - See installation commands below
   * - **USB cable**
     - Micro-USB (provided with LP-AM263P)
     - Powers the board + XDS110 JTAG + UART

Optional but recommended:

* `Windows Terminal <https://aka.ms/terminal>`_ (much better than cmd/PowerShell window)
* Tera Term or PuTTY (for serial console)
* Chocolatey (for easy cmake/ninja install)

Step 1: Install Build Tools
===========================

Open **PowerShell as Administrator** and run:

.. code-block:: powershell

   # 1. Chocolatey (package manager) — skip if you prefer manual installs
   Set-ExecutionPolicy Bypass -Scope Process -Force
   [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
   iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

   # 2. Core build tools
   choco install -y cmake ninja gperf dtc-msys2 wget 7zip

   # 3. (Optional) Visual Studio Build Tools or "Desktop development with C++"
   #    if you ever need to compile host tools.

Close and reopen PowerShell after the above.

Step 2: Clone + West Workspace (One Time)
=========================================

From your normal user PowerShell:

.. code-block:: powershell

   # Choose a parent directory (example: C:\zephyr-workspace)
   mkdir C:\zephyr-workspace
   cd C:\zephyr-workspace

   # Clone the fork (this repo contains both manifest and Zephyr tree)
   git clone https://github.com/abhinav937/zephyr-ti.git

   # Recommended Git settings for Zephyr on Windows (run once)
   git config --global core.autocrlf false
   git config --global core.eol lf
   # If you ever hit "Filename too long" errors:
   # git config --global core.longpaths true

   # Create isolated Python environment (strongly recommended on Windows)
   python -m venv .venv-zephyr
   .\.venv-zephyr\Scripts\activate

   # Install west and initialize using the local tree as manifest
   pip install west
   west init -l zephyr-ti

   # Pull minimal dependencies (narrow + shallow for speed)
   west update --narrow -o=--depth=1 cmsis

   # Zephyr Python requirements
   pip install -r zephyr-ti\scripts\requirements.txt

   # If you are already inside an existing clone (e.g. C:\zephyr-ti) and
   # just want to use it directly as the workspace root, you can also do:
   #   cd C:\zephyr-ti
   #   python -m venv .venv-zephyr
   #   .\.venv-zephyr\Scripts\activate
   #   pip install west
   #   west init -l .
   #   ... (rest of the steps, replacing "zephyr-ti\" with ".\" where needed)

Step 3: Install Zephyr SDK Toolchain
====================================

Still inside the activated venv:

.. code-block:: powershell

   # This downloads ~1.2 GB the first time (arm-zephyr-eabi GCC 14.x)
   west sdk install -t arm-zephyr-eabi

   # Verify it worked
   arm-zephyr-eabi-gcc --version

You should see GCC 14.x from the Zephyr SDK.

Step 4: Build hello_world
=========================

.. code-block:: powershell

   # Activate the environment (every new PowerShell window)
   cd C:\zephyr-workspace
   .\.venv-zephyr\Scripts\activate
   $env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

   # Clean build for the exact board target
   west build -b lp_am263p/am263p4/r5f0_0 zephyr-ti\samples\hello_world -p always

   # If you are working directly inside the C:\zephyr-ti clone:
   #   cd C:\zephyr-ti
   #   .\.venv-zephyr\Scripts\activate
   #   $env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
   #   west build -b lp_am263p/am263p4/r5f0_0 samples\hello_world -p always

Expected output (last lines):

.. code-block::

   [100%] Linking C executable zephyr\zephyr.elf
   Memory region         Used Size  Region Size  %age Used
   OCRAM:       39848 B       256 KB     15.15%
   ...
   [100%] Built target zephyr.elf

The ELF is at:

.. code-block::

   C:\zephyr-workspace\build\zephyr\zephyr.elf

Verify the build quickly:

.. code-block:: powershell

   arm-zephyr-eabi-size build\zephyr\zephyr.elf
   arm-zephyr-eabi-readelf -h build\zephyr\zephyr.elf | findstr "Entry|Machine"

Entry point should be in the OCRAM range and machine should be ARM.

**Incremental builds** (after the first successful build):

.. code-block:: powershell

   west build

**Full clean rebuild** (when you change SoC/board Kconfig or DTS):

.. code-block:: powershell

   west build -b lp_am263p/am263p4/r5f0_0 zephyr-ti\samples\hello_world -p always

   # Inside C:\zephyr-ti directly:
   #   west build -b lp_am263p/am263p4/r5f0_0 samples\hello_world -p always

The build directory lives at ``C:\zephyr-workspace\build`` (or ``C:\zephyr-ti\build``
if you initialized west directly inside the clone). You can delete it entirely
for a completely fresh start.

Hardware: Connect the LP-AM263P
===============================

Before plugging in:

1. Install **Code Composer Studio** (https://www.ti.com/tool/CCSTUDIO)
   - During install, select at least "Sitara AM26x / AM263x" device support.
2. Plug the Micro-USB cable into the LP-AM263P (the one near the XDS110 section).
3. Windows should enumerate:
   - XDS110 debug probe (JTAG)
   - XDS110 Class Application/User UART (this is your console COM port)

Check in **Device Manager** → Ports (COM & LPT). Note the COM number (e.g. COM5).

If you do **not** see any XDS110 entries, install the standalone XDS110 drivers
from TI or just launch CCS once — it usually installs them.

Running Zephyr via CCS (Recommended for First Bring-up)
=======================================================

This is currently the most reliable way to load and run on the AM263P4.

1. Launch **Code Composer Studio**.
2. Create a new Target Configuration:
   - *File → New → Target Configuration File*
   - Connection: **Texas Instruments XDS110 USB Debug Probe**
   - Board or Device: search for **AM263P4** or **AM263Px**
   - Save as ``LP-AM263P.ccxml``
3. Launch the target:
   - In the Target Configurations view, right-click your .ccxml → **Launch Selected Configuration**
4. Connect to the core:
   - In the Debug view you should see **R5FSS0_0** (and possibly others).
   - Right-click **R5FSS0_0** → **Connect Target**
5. Load the Zephyr ELF:
   - *Run → Load → Load Program...*
   - Browse to one of:
     ``C:\zephyr-workspace\build\zephyr\zephyr.elf``
     or (if working inside the clone directly) ``C:\zephyr-ti\build\zephyr\zephyr.elf``
   - Click **OK** / **Finish**
6. Open a terminal for UART output:
   - Use **Tera Term** (recommended), PuTTY, or VS Code Serial Monitor.
   - Select the XDS110 COM port, 115200, 8N1, no flow control.
   - Open the port **before** running on the target.
7. Run the program:
   - *Run → Resume* (F8) or the green play button.
   - You should see:
     ``Hello World! lp_am263p/am263p4/r5f0_0``

If you see nothing or garbage, go straight to the Troubleshooting section below.

.. _Status and verify-list:

Troubleshooting & Verify-List (First Silicon)
*********************************************

Common Windows / first-run issues:

**No COM port appears in Device Manager**
   - Install CCS (it brings XDS110 USB drivers).
   - Try a different USB cable / port.
   - Run ``C:\ti\ccs_base\emulation\drivers\xds110reset.bat`` (if present).

**"Hello World" never prints**
   1. UART0 pin mux (most likely on ControlCARD).
      The current port has **no pinctrl-0** on UART0. It relies on the
      boot ROM / SBL having already muxed the UART pins.
      On the TMDSCNCD263P ControlCARD the pin routing can differ from the
      LaunchPad.
   2. Wrong baud rate divisor.
      On the TMDSCNCD263P ControlCARD the gel shows LIN0_UART0 at **160 MHz**.
      Update ``clock-frequency`` in ``dts/arm/ti/am263p4.dtsi`` (currently
      set to 160000000 after ControlCARD measurement) if your board differs.

**Garbled / wrong characters**
   - Same as above — adjust ``clock-frequency`` on the ``&uart0`` node and rebuild.

**Board runs but timing is crazy** (``k_sleep(1000)`` is not 1 second)
   - RTI input clock assumption is wrong.
   - On the TMDSCNCD263P ControlCARD the gel shows RTI timers at **200 MHz**.
   - Current default in ``soc/ti/am263x/Kconfig.defconfig`` is now 200000000
     (updated after ControlCARD measurement). Adjust if your board differs.

**Hangs after first tick or no periodic interrupts**
   - RTI prescale / COMP0 configuration issue in ``ti_rti_timer.c``.
   - The driver writes ``RTICPUC0 = 0`` (divide-by-1). Confirm this matches the
     hardware default and TRM description for your clock tree.

**CCS says "No source available" or refuses to load**
   - Make sure you are connected to **R5FSS0_0** specifically.
   - The ELF has no DWARF path issues on a clean SDK build.

**Build fails with "DT_FREQ_M not found" or similar**
   - You are on an older tree. Pull the latest commits (the fix in
     ``a0d48d2`` added the missing ``<freq.h>`` include).

Quick Verification Commands (after loading)
===========================================

Once you have console output, try these samples in order:

1. ``samples/hello_world`` — basic smoke test (done)
2. ``samples/synchronization`` — tests sleeping + threads
3. ``samples/basic/blinky`` (once GPIO works) — validates pin control + GPIO driver
4. Custom test: add a ``k_sleep(K_MSEC(500))`` loop and toggle a GPIO you can
   measure with a scope/logic analyzer.

Next Steps After First Successful Boot
======================================

- Record the real UART clock and RTI clock values you had to use.
- Update ``dts/arm/ti/am263p4.dtsi`` and ``soc/ti/am263x/Kconfig.defconfig``.
- Add a proper pinctrl driver + pinmux for UART0 (and later EPWM/ADC).
- Implement flash/SBL boot path (currently only debugger load is supported).
- Add OpenOCD support once the correct target config is known.

References
==========

- Full porting plan: ``docs/PLAN-am263p.md``
- Verified memory map & peripheral bases: ``docs/am263p-memory-map.md``
- AM263Px TRM: SPRUJ55 (https://www.ti.com/lit/ug/spruj55c/spruj55c.pdf)
- TI MCU+ SDK (register truth): https://github.com/TexasInstruments/mcupsdk-core

Report any findings (working clocks, required changes, etc.) in the repo issues
or as a follow-up PR. This port is being built in the open. Your hardware runs
are extremely valuable.

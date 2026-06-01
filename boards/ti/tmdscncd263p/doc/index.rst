.. _tmdscncd263p:

TI TMDSCNCD263P AM263P ControlCARD
##################################

Overview
********

Zephyr support for the **TI TMDSCNCD263P AM263P ControlCARD** (AM263P4, quad
Cortex-R5F @ 400 MHz). Target: ``tmdscncd263p/am263p4/r5f0_0`` (R5FSS0 Core 0).

The image is **RAM-loaded** (TCM + OCRAM @ ``0x70040000``) over the onboard
XDS110 probe — it is not flashed. Connect **Core_R5_0 only**.

Verified working on hardware
============================

* **Console** — UART0 to the XDS110 virtual COM port, 115200 8N1, via DT
  pinctrl (``ti,am263x-pinctrl``).
* **System tick** — ``ti_rti_timer`` (RTI0, 200 MHz input). ``k_msleep`` /
  ``k_uptime_get`` confirmed accurate.
* **User LEDs** — LD6 (GPIO22) and LD7 (GPIO58) via the ``ti,davinci-gpio``
  driver, pads muxed to GPIO mode 7 through DT pinctrl. The ``blinky`` pattern
  in ``samples/hello_world`` alternates them at 500 ms.

Supported hardware
===================

.. list-table::
   :header-rows: 1

   * - Feature
     - Controller
     - Driver / binding
   * - Console UART
     - UART0
     - ``ns16550`` + ``ti,am263x-pinctrl``
   * - System timer
     - RTI0
     - ``ti,rti-timer``
   * - GPIO (LEDs)
     - GPIO0 bank01 / bank23
     - ``ti,davinci-gpio``
   * - Interrupt controller
     - VIM
     - (reused arch VIM driver)

Building and running
********************

.. code-block:: powershell

   west build -b tmdscncd263p/am263p4/r5f0_0 samples/hello_world

On Windows in this workspace, ``.\build.ps1`` builds this target directly
(it pins ``ZEPHYR_BASE`` to work around ``.west/`` living at the ``C:\`` root).

Loading over XDS110 (TI CCS)
============================

#. Connect **only Core_R5_0** in your target configuration.
#. **Load Program** → ``build/zephyr/zephyr.elf``.
#. **Restart**, then **Resume**.
#. Console output appears on the XDS110 user-UART COM port at 115200 8N1.

If you hit the ``-215`` "semaphore time-out", a stale ``DSLite`` debug server
owns the probe — kill it and replug the XDS110 (see ``debug/ccs/``).

References
==========

* AM263Px datasheet: SPRSP81
* AM263Px TRM: SPRUJ55
* ControlCARD user guide: SPRUJ86
* Port status and roadmap: ``porting/ROADMAP.md``

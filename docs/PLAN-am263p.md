# Porting Plan: Zephyr on TI AM263P (LP-AM263P)

**Target:** AM263P4 (Sitara AM263Px real-time MCU), quad Arm Cortex-R5F @ 400 MHz
**Board:** LP-AM263P LaunchPad (XDS110 onboard debug probe)
**First milestone:** `hello_world` over UART0 on R5F SubSystem 0, Core 0
**Why this is tractable (unlike C2000):** Cortex-R5F is ARMv7-R — 8-bit bytes,
standard GCC/clang via the Zephyr SDK, and the architecture is **already
supported in this tree**. This is a normal SoC + board bring-up, not an
architecture port.

---

## 1. What we reuse vs. what we build

### Already in this Zephyr tree — reuse as-is
| Component | Path | Notes |
|---|---|---|
| Cortex-R5F arch | `arch/arm/core/cortex_a_r/` | reset, context switch, MPU, TCM, cache, exceptions — all done |
| **VIM interrupt controller** | `drivers/interrupt_controller/intc_vim.c` + `dts/bindings/interrupt-controller/ti,vim.yaml` | AM263x uses the **same VIM** — huge win |
| K3 Cortex-R SoC skeleton | `soc/ti/k3/common/cortex_r/` | template for soc.c, linker.ld, MPU regions |
| AM243x R5F board | `boards/ti/am243x_evm/*_r5f0_0*` | template for board DTS, pinctrl, defconfig, openocd.cfg |
| AM64x R5F SoC dtsi | `dts/arm/ti/am64x_r5*.dtsi` | template for SoC devicetree structure |

### Build new
| Component | Path | Why new |
|---|---|---|
| SoC layer | `soc/ti/am263x/` | AM263x is **not K3** — no DMSC/system-firmware handshake, no `ctrl_partitions` unlock. Boot is *simpler* than AM243x. Different clock/PLL + memory map. |
| SoC devicetree | `dts/arm/ti/am263p*.dtsi` | new memory map + peripheral addresses |
| Board | `boards/ti/lp_am263p/` | new board files |
| EPWM / ADC drivers | `drivers/pwm/`, `drivers/adc/` | AM263x motor-control peripherals differ from existing TI drivers (later phase) |

**Key architectural difference from AM243x:** the K3 parts (AM243x/AM64x) boot
via a DMSC system controller running TI SYSFW, and Zephyr must negotiate control
partitions (`k3_unlock_all_ctrl_partitions()`). **The AM263x has no DMSC** — the
R5F is the primary core and boots directly. So we *drop* the K3 ctrl_partitions
and secure_proxy/dmsc machinery, which makes our soc.c simpler than the K3 one.

---

## 2. Source of truth — where every value comes from

Do **not** guess addresses. Every base address, IRQ number, and clock value is
pulled from these, in priority order:

1. **AM263Px Technical Reference Manual — SPRUJ55** (Rev. B/C)
   `https://www.ti.com/lit/ug/spruj55c/spruj55c.pdf`
   — memory map, peripheral base addresses, VIM, clock tree, boot flow.
2. **AM263Px Register Addendum — SPRUJ57** — exhaustive register/bitfield tables.
3. **AM263P4 datasheet** — `https://www.ti.com/product/AM263P4` — pin mux, package, electricals.
4. **TI MCU+ SDK (`mcupsdk-core`)** — `https://github.com/TexasInstruments/mcupsdk-core`
   — the *practical* source for exact base addresses: SysConfig-generated
   headers (`drivers/soc/am263px/`, `cslr_soc_baseaddress.h`-style files) list
   every peripheral base. Cross-check against the TRM.
5. **AM263x TRM SPRUJ17** — predecessor part, useful when SPRUJ55 is ambiguous
   (the AM263P is largely a superset of AM263x).

> Note AM263**x** (SPRUJ17) vs AM263**Px** (SPRUJ55) are closely related but not
> identical — always confirm against the SPRUJ55 (the "P" part) for our target.

---

## 3. Memory map (known structure; exact addresses to confirm from TRM)

AM263P4 R5F subsystem memory (from TI docs):

| Region | Size | Purpose | Zephyr chosen node |
|---|---|---|---|
| TCMA | 64 KB | fast code (vectors, hot ISRs) | `zephyr,sram1` candidate |
| TCMB | 192 KB | fast data / stacks | — |
| TCM total | 256 KB per core (single-core/lockstep mode) | | |
| OCSRAM | up to 3 MB shared | main RAM (.text/.data/.bss/heap) | `zephyr,sram` |

- TCMA base is typically `0x00000000` (R5F view), TCMB follows — **confirm exact
  TCMB base from SPRUJ55** (there's a known TCMB map erratum on AM263P4-Q1; check
  the TI E2E note during impl).
- OCSRAM base + size: confirm from SPRUJ55 memory map.
- First boot strategy: link everything into OCSRAM (simplest), put vector table
  in TCMA. Optimize TCM placement later.

---

## 4. Phase plan (mirrors the task list)

### Phase 0 — References (task #16)
Pull SPRUJ55 + MCU+ SDK headers; record exact base addresses for VIM, UART0,
RTI timer, GPIO, clock/PLL, EPWM, ADC into a scratch table. Gate for everything.

### Phase 1 — Make it build (tasks #17, #18)
- `soc/ti/am263x/`: Kconfig.soc (`SOC_SERIES_AM263X`, `SOC_AM263P4`), defconfig,
  soc.c (early init: cache + MPU enable, clock/PLL — **no** K3 ctrl_partitions),
  `arm_mpu_regions.c`, `linker.ld` (OCSRAM + TCM), soc.yml.
- `dts/arm/ti/am263p.dtsi` + `am263p_r5f0_0.dtsi`: CPU, TCM, OCSRAM, `&vim`,
  UART0, RTI timer, GPIO nodes.

### Phase 2 — First boot (tasks #19, #20, #21, #22)
- `boards/ti/lp_am263p/`: DTS (includes SoC dtsi + pinctrl), `-pinctrl.dtsi`
  (UART0 TX/RX), `_defconfig`, `.yaml`, `support/openocd.cfg` (XDS110), doc.
- UART console: reuse existing TI/ns16550-class driver — likely just DT wiring.
- System timer: reuse existing TI timer driver (RTI on AM26x) for the Zephyr tick.
- **Milestone:** `hello_world` + `synchronization` on R5FSS0 core0 over UART0.

### Phase 3 — Peripherals (tasks #23, #24)
- GPIO + `blinky` (validates pinctrl + GPIO).
- EPWM + ADC drivers — the motor-control payoff.

---

## 5. Toolchain & flashing

- **Compiler:** Zephyr SDK `arm-zephyr-eabi` (GCC) — standard, no special setup.
  (TI's `ti-arm-clang` exists in the MCU+ SDK but we use the Zephyr SDK.)
- **Flash/debug:** XDS110 on the LaunchPad. Path A: **OpenOCD** (model the
  `boards/ti/am243x_evm/support/openocd.cfg`). Path B: **CCS / DSLite / Uniflash**
  if OpenOCD lacks a clean AM263P target. Confirm during board task.
- R5F boot: AM263x boot ROM loads from flash/SBL; for development we load to
  OCSRAM via the debugger and run. SBL/flash-boot is a later concern.

---

## 6. Open questions to resolve during Phase 0/1

1. Exact TCMB base address (watch the AM263P4-Q1 TCMB errata on TI E2E).
2. Is there an existing Zephyr driver that matches the AM263x UART exactly, or do
   we add a `compatible` to ns16550? (Check `drivers/serial/`.)
3. AM263x system-tick timer: which RTI/timer instance, and does an existing
   Zephyr `drivers/timer/` driver cover it?
4. Clock/PLL: minimum init to get UART0 + R5F at a known frequency. (MCU+ SDK
   `SOC_*` clock code is the reference.)
5. Single-core vs lockstep R5F mode for first boot (default to single-core).

---

## References (links)

- AM263Px TRM (SPRUJ55): https://www.ti.com/lit/ug/spruj55c/spruj55c.pdf
- AM263x TRM (SPRUJ17): https://www.ti.com/lit/ug/spruj17i/spruj17i.pdf
- AM263P4 product / datasheet: https://www.ti.com/product/AM263P4
- LP-AM263P board: https://www.ti.com/tool/LP-AM263P
- TI MCU+ SDK (register defs, clock code): https://github.com/TexasInstruments/mcupsdk-core
- Zephyr VIM driver: `drivers/interrupt_controller/intc_vim.c`
- Reference port (AM243x R5F): `boards/ti/am243x_evm/`, `soc/ti/k3/common/cortex_r/`

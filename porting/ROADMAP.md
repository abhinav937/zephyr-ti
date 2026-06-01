# AM263P Zephyr Port — Roadmap

Living plan for the TI AM263P port and the motor-control platform built on it.
Source of truth for sequencing and status. Updated on **every commit** (see the
Changelog at the bottom; the convention is recorded in `/CLAUDE.md`).

## Context

This repo is a **fork of Zephyr** that adds **TI AM263P** support. AM263P is
**not** in upstream Zephyr — it was added here: early `soc/ti/am263x` +
`boards/ti/lp_am263p`, running on R5FSS0 core0. The port is being validated on
the **TMDSCNCD263P ControlCARD** (not the LaunchPad).

Larger goal: an **AMDC-style, student-friendly motor-control platform** on top
of this port, kept **portable to future TI AM26x boards** so that retargeting is
a devicetree/BSP change, not a code rewrite.

## Architecture — two-layer model (the portability contract)

```
Layer 1 — the Zephyr port (THIS repo, "Zephyr's original way")
  soc/ti/am263x/        SoC family: clocks, MPU, core, Kconfig
  boards/ti/<board>/    per physical board (lp_am263p, future tmdscncd263p, ...)
  dts/arm/ti/*.dtsi     SoC devicetree (peripheral addresses, IRQs)
  dts/bindings/...      compatibles: "ti,am263x-*"
  drivers/...           generic, DT-bound drivers (gpio, pinctrl, uart, pwm...)
  Goal: upstreamable. Nothing board- or app-specific leaks in.

Layer 2 — the AMDC-like platform (SEPARATE out-of-tree workspace, future)
  apps/<name>/{app_,task_,cmd_}, common/framework, common/utils
  Consumes Layer 1 via west.yml. Board-agnostic: uses DT aliases/chosen +
  driver APIs only. Retarget = `west build -b <board>`. No code change.
```

**Rule:** a driver bound by compatible string is reusable on any board/SoC that
instantiates that IP in its devicetree. If AM263P is replaced by another AM26x
part: add a `soc/` + `boards/` entry + a `.dtsi`; the drivers and the entire
Layer-2 app stay untouched. Do **not** bake app logic into the Zephyr tree.

## Phased roadmap

Status: `[x]` done · `[~]` in progress · `[ ]` not started

- [x] **Boot** — kernel boots to `main` on R5FSS0 core0 (confirmed in CCS).
- [x] **GPIO / IOMUX proven** — LED blink on LD7/GPIO58 (and LD6/GPIO22 wired),
      pinmux + GPIO register model verified on hardware.
- [x] **RTI system-timer driver present** — needs tick verification (Phase 4).
- [ ] **Phase 1 — UART console** *(next)*: pinmux UART0 TX/RX pads so
      `printk`/console reaches the XDS110 virtual COM port. Unblocks log-based
      debugging.
- [ ] **Phase 2 — `pinctrl-am263x` driver + DT bindings**: move hand-coded pad
      config into the Zephyr pinctrl model (`pinctrl-0` in `.dts`). Highest
      leverage for portability.
- [ ] **Phase 3 — real `tmdscncd263p` board + proven GPIO driver**: split the
      ControlCARD from the LaunchPad files; correct LEDs/clocks; confirm or
      replace the GPIO driver.
- [ ] **Phase 4 — system timer verified**: confirm RTI tick so
      `k_msleep`/`k_timer`/scheduling are correct; drop the bring-up busy-wait.
- [ ] **Phase 5 — single-core app skeleton (Layer 2)**: blinky + Shell, the
      `app_/task_/cmd_` pattern, Kconfig app-select. Student-template milestone.
- [ ] **Phase 6 — motor peripherals**: ePWM driver → ADC driver → PWM-synced
      control ISR (custom drivers; no upstream Zephyr equivalents yet).
- [ ] **Phase 7 — dual-core AMP**: sysbuild + IPC (`ipc_service` + mbox);
      comms/shell on one core, deterministic control loop on another (the full
      AMDC topology).

Phases 1–4 are Layer-1 port work; 5–7 build the Layer-2 platform.

## Open questions to verify from docs (do not assume)

- **GPIO instance mapping**: GPIO0 `0x52000000` vs GPIO1 `0x52001000` boundary,
  and how GPIO>127 (e.g. GPIO136/137) map. Confirm `ti,davinci-gpio` actually
  matches the AM263 GPIO IP. → TRM (SPRUJ55) GPIO chapter.
- **Multi-core boot/SBL flow**: how each core's image is loaded (debugger vs
  SBL). Affects the AMP story (Phase 7).
- **sysbuild** feasibility for the AM263 multi-core build, or whether we drive
  per-core builds manually first.

(Mirrors the "Still to verify in the TRM" list in `/CLAUDE.md`.)

## Changelog

Reverse-chronological. One entry per commit: `### <date> — <commit subject>`,
then bullets of what changed and the phase touched.

### 2026-05-31 — docs(am263p): porting roadmap, working rules, vendor docs; verify LD6/GPIO22
- Added `porting/ROADMAP.md` (this file): context, two-layer architecture,
  phased roadmap with status, open TRM questions, changelog.
- Added `CLAUDE.md`: no-assume rules, verified-facts ledger, two-layer model,
  roadmap-upkeep workflow.
- Added `vendor-docs/`: datasheet SPRSP81, TRM SPRUJ55, ControlCARD guide
  SPRUJ86 + index README.
- `samples/hello_world`: wired LD6/GPIO22 (pad LIN2_TXD `0x53100058`, mode 7)
  alongside LD7/GPIO58 — values verified from SPRSP81 Table 5-1.
- Phase touched: process/docs + GPIO bring-up (pre-Phase-1).

### 2026-05-31 — bring-up: ControlCARD LED blink + build robustness (commit 155c4007, backfilled)
- `samples/hello_world` LED blink rewrite (LD7/GPIO58 bank fix, busy-wait).
- `build.ps1` pins `ZEPHYR_BASE` (fixes `WEST_TOPDIR=C:\` "Could not find Zephyr
  package"); added build helper scripts.
- (Committed before this changelog existed; recorded here for history.)

### (earlier) — initial AM263P scaffolding
- `dts/arm/ti/am263p4.dtsi` SoC devicetree; `boards/ti/lp_am263p` board (R5FSS0
  core0); TI RTI system-clock driver; `soc/ti/am263x` Kconfig; first-build fixes
  so `hello_world` links and boots.
- Phase touched: boot bring-up.

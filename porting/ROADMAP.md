# AM263P Zephyr Port — Roadmap

Living plan for the TI AM263P port and the motor-control platform built on it.
Source of truth for sequencing and status. Updated on **every commit** (see the
Changelog at the bottom; the convention is recorded in `/CLAUDE.md`).

**Detailed implementer hand-off:** see `porting/IMPLEMENTATION_PLAN.md` (verified
hardware facts, pin tables, existing code map, per-phase executable steps, and
the two-layer portability rules). This ROADMAP is the status tracker; the
Implementation Plan is the "what exactly to do" document.

## Next up (resume here)

**Phase 4 — verify the RTI system timer.** Now that the console works, this is
the fast, high-value next step: it confirms `k_msleep`/`k_timer`/scheduling are
trustworthy and removes the bring-up `busy_delay()` crutch.

Plan:
1. `samples/hello_world/src/main.c`: replace the two `busy_delay()` calls with
   `k_msleep(500)`, and change the heartbeat to print `k_uptime_get()` (ms),
   e.g. `printk("uptime %lld ms, beat %u\n", k_uptime_get(), beat++);`.
   Remove `busy_delay()` if then unused.
2. Build (`.\build.ps1`), load `build/zephyr/zephyr.elf` in CCS → Restart →
   Resume, watch the 115200 console.
3. **Pass:** uptime advances ~1000 ms per full LD6→LD7 cycle (two 500 ms
   sleeps), stable and matching wall-clock; LEDs still alternate.
4. **If it hangs or timing is wildly off:** the RTI driver/clock is the
   suspect — check the TI RTI timer driver (commit 10626c28c84), the `rti0`
   node in `dts/arm/ti/am263p4.dtsi` (`0x52180000`, VIM IRQ 84), and
   `CONFIG_SYS_CLOCK_*`. Confirm the RTI input clock (GEL showed 200 MHz)
   matches what the driver assumes.
5. On success: flip Phase 4 to `[x]`, add a Changelog entry, commit.

Resume context: read `/CLAUDE.md` (rules + verified-facts ledger) and this
file. Hardware docs are in `vendor-docs/` (datasheet SPRSP81, TRM SPRUJ55,
ControlCARD guide SPRUJ86). Build with `.\build.ps1`; load over XDS110 (connect
Core_R5_0 only). Console = XDS110 user-UART COM port, 115200 8N1.

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
- [x] **Phase 1 — UART console** *(verified on hardware 2026-05-31)*:
      UART0 TX/RX pads muxed to mode 0 via DT pinctrl
      (`CONFIG_PINCTRL` + `pinctrl-0` on `&uart0`); `printk`/console reaches the
      XDS110 virtual COM port at 115200 8N1. Boot banner + heartbeat confirmed.
      Driven by the existing `pinctrl_ti_am263x` driver; ns16550 applies it
      before console use. (RX/shell input not yet exercised — see Phase 5.)
- [x] **Phase 2 — `pinctrl-am263x` driver + DT bindings**: `ti,am263x-pinctrl`
      binding + `pinctrl_ti_am263x` driver proven during Phase 1 (UART0 pads);
      the LED bring-up register pokes in `main.c` are now migrated to DT pinctrl
      (`led_ld6_default`/`led_ld7_default` on `&gpio0`/`&gpio1`), so no raw pad
      writes remain.
- [x] **Phase 3 — real `tmdscncd263p` board + proven GPIO driver**: GPIO driver
      proven (LD6/LD7 via `ti,davinci-gpio` + DT pinctrl), and the ControlCARD
      now has its own `boards/ti/tmdscncd263p` split out from the LaunchPad
      `lp_am263p` files. Verified on hardware: builds + boots + blinks on
      `tmdscncd263p/am263p4/r5f0_0`.
- [x] **Phase 4 — system timer verified**: RTI tick works; `k_msleep` and
      `k_uptime_get()` advance. Root cause of the earlier hang: `CPUC0=0`
      (divide-by-2³², ~21 s/tick) instead of `CPUC0=1` (divide-by-1, 200 MHz).
- [~] **Phase 5 — single-core app skeleton (Layer 2)**: in-tree skeleton done
      (`samples/am263_app`: `app_/task_/cmd_` pattern, Kconfig app-select, blink
      task + Zephyr shell on UART0 — UART RX proven). Remaining: extract the
      framework to the out-of-tree Layer-2 workspace consumed via `west.yml`.
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

### 2026-05-31 — feat(am263p): Phase 5 app skeleton (samples/am263_app) + shell on UART0
- Proved UART RX end-to-end: enabled the Zephyr shell on UART0 (the chosen
  `zephyr,shell-uart`); `help`, `am263 uptime`, tab-completion all work.
- Added `samples/am263_app` — the in-tree seed of the AMDC-style Layer-2 layout:
  `framework/` (`apps_init()` + `am263` command group), `apps/blink/`
  (`app_blink` / `task_blink` thread / `cmd_blink` group), Kconfig app-select
  (`AM263_APP_BLINK`), `main()` only calls `apps_init()`. `blink on|off|rate|status`
  control the LED task at runtime.
- `prj.conf`: `CONFIG_SHELL_VT100_COMMANDS=n` for plain-text output on the
  non-VT100 console terminal (also disables colors + cursor-move escapes).
- `samples/hello_world` restored to the minimal no-shell smoke test.
- `build.ps1` default sample -> `samples\am263_app` (`-Sample samples\hello_world`
  for the smoke test).
- Verified on hardware: clean shell, blink task + runtime rate control.
- Remaining for Phase 5: extract the framework out-of-tree (Layer-2 workspace).

### 2026-05-31 — feat(am263p): tmdscncd263p ControlCARD board; simpler build.ps1 (Phase 3 done)
- Added `boards/ti/tmdscncd263p` (board.yml, Kconfig, `_r5f0_0` dts/yaml/defconfig,
  support/openocd.cfg, fresh doc) split out from the LaunchPad `lp_am263p` files.
  All pin/LED/UART/RTI facts in it are ControlCARD-verified on hardware. `lp_am263p`
  is left as the separate (currently unvalidated) LaunchPad board.
- `build.ps1`: retargeted default to `tmdscncd263p/am263p4/r5f0_0` and rewritten —
  `-Board`/`-Sample` are parameters again; the homegrown "aggressive rebuild"
  (delete .obj + touch soc.c/main.c + git-scan) is removed in favour of
  `west build -p auto` (west does a pristine only when the build dir can't be
  reused, e.g. a board switch). Kept the SDK/CMake auto-detection + ZEPHYR_BASE pin.
- Verified on hardware: clean build + boot + LED blink on the new board.

### 2026-05-31 — feat(am263p): LEDs via ti,davinci-gpio driver + DT pinctrl (Phase 2 done, Phase 3 GPIO proven)
- Correct GPIO model: per TRM SPRUJ55 §13.1.1 a single GPIO instance
  (GPIO0 @ 0x52000000 for R5FSS0-core0) covers all signals via banked register
  *pairs* (`*_DATA01` = signals 0..31, `*_DATA23` = 32..63), and there are four
  per-core instances (GPIO0..3) — not a pin-range split. Corrects the old
  "GPIO0 vs GPIO1 = low vs high pins" assumption in the to-verify list.
- `dts/arm/ti/am263p4.dtsi`: replaced the 32-pin placeholder `gpio0` with two
  `ti,davinci-gpio` bank nodes whose `reg` points at each bank pair's DIR so the
  flat driver struct lines up: `gpio0 @0x52000010` (bank01, signals 0..31),
  `gpio1 @0x52000038` (bank23, signals 32..63). Both default `disabled`.
  DIR/OUT/SET/CLR offsets verified; in_data/trigger regs left UNVERIFIED
  (output-only) pending the TRM GPIO register-map table.
- `boards/ti/lp_am263p/...dts`: LD7/GPIO58 -> `&gpio1 26`; added
  `led_ld6_default`/`led_ld7_default` pad-mux (mode7, `0x107`) and enabled
  `&gpio0`/`&gpio1` with their `pinctrl-0`. Closes the Phase 2 remainder.
- `samples/hello_world/src/main.c`: rewritten to the GPIO API (`gpio_dt_spec`
  from `DT_ALIAS(led0/led1)`); all raw GPIO/pinmux register pokes removed.
- Verified on hardware: LD6/LD7 alternate at 500 ms, `uptime` advances. (An
  UNDEFINED-INSTRUCTION abort at PC=0x70040000 = `_vector_table` seen while
  using the debugger is a JTAG halt/resume-from-WFI artifact, not a fault.)

### 2026-05-31 — chore(am263p): build robustness + CCS debug configs
- `build.ps1`: auto-detect the Zephyr SDK (env var → `zephyr-sdk-*` under
  `$HOME`/`C:\` → legacy path) and CMake; lock board/sample to the AM263P
  bring-up target; aggressive force-rebuild of `soc.c`/`main.c` to work around
  unreliable CMake dependency tracking on this machine; clearer next-step output.
- `debug/ccs/`: checked-in CCS launch/target configs (`.ccxml`, `.theia`),
  `refresh-elf.ps1`, and `kill-xds110.cmd` for the `-215` probe-lock error.
  Nested `.gitignore` keeps `zephyr.elf`/`*.launch`/binaries out of git.
- Phase touched: tooling/process (no firmware impact).

### 2026-05-31 — fix(am263p): RTI system timer — CPUC0 prescale off-by-one (Phase 4)
- `drivers/timer/ti_rti_timer.c`: set `CPUC0 = 1` (was `0`). Per TRM SPRUJ55
  §13.5.1.4.3, UC0 counts up until it equals CPUC0, then FRC0 increments and
  UC0 resets. `CPUC0 = 0` only matches on UC0's full 32-bit wrap, so FRC0
  ticked once per ~21 s — the kernel tick (COMP0 ≈ 200k cycles) never fired,
  hanging `k_msleep`. `CPUC0 = 1` makes FRC0 advance every input clock (200 MHz).
- Verified on hardware: FRC0 jumps ~185M counts per short delay; `k_uptime_get()`
  advances. Phase 4 complete.
- `samples/hello_world/src/main.c`: removed all RTI debug instrumentation;
  restored a clean `k_msleep(500)` LED-blink loop.
- `soc/ti/am263x/soc.c`: re-enabled `am263p_enable_rti_clock()` (GEL-less boots
  need it) and stripped its verbose debug printks.

### 2026-06-01 — docs: add comprehensive Implementation Plan hand-off document
- Created `porting/IMPLEMENTATION_PLAN.md`: self-contained implementer guide
  containing the full verified hardware facts (with sources), pin table,
  existing code map, two-layer architecture rules, and detailed executable
  steps for Phases 3–7.
- Updated this ROADMAP.md to reference the new plan as the primary "what to
  do" artifact (this file remains the status + changelog source of truth).
- Goal: future Grok/Claude sessions can read the plan directly from disk
  instead of relying on long context.
- Phase touched: process/docs (no code or build impact).

### 2026-05-31 — docs(am263p): record Phase 4 plan; mark Phase 2 mechanism proven
- Added a "Next up (resume here)" section with the executable Phase 4 plan
  (verify RTI timer, swap `busy_delay()` → `k_msleep`, check `k_uptime_get()`).
- Marked Phase 2 `[~]` (pinctrl mechanism proven in Phase 1; LED migration left).
- Phase touched: process/docs (planning hand-off; no code/build impact).

### 2026-05-31 — feat(am263p): Phase 1 UART0 console via DT pinctrl
- New binding `dts/bindings/pinctrl/ti,am263x-pinctrl.yaml` (`pinmux = <reg val>`).
- `dts/arm/ti/am263p4.dtsi`: add `pinctrl@53100000` controller node.
- Board `lp_am263p` `.dts`: UART0 RX/TX pin nodes (A7 `0x5310006c`=`0x110`,
  A6 `0x53100070`=`0x100`, mux mode 0 per SPRSP81 Table 5-1) + `pinctrl-0`/
  `pinctrl-names` on `&uart0`.
- `..._defconfig`: `CONFIG_PINCTRL=y` (auto-enables `PINCTRL_TI_AM263X`).
- `samples/hello_world`: add console banner + heartbeat printk.
- Uses existing `drivers/pinctrl/pinctrl_ti_am263x.c`; ns16550 applies pinctrl
  at init. RX force-input-enable bit (0x10) provisional until shell input is
  exercised (Phase 5).
- **Verified on hardware (2026-05-31):** boot banner + custom banner + heartbeat
  print at 115200 8N1 on the XDS110 user-UART COM port.
- Phase touched: **Phase 1 (UART console) — DONE.**

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

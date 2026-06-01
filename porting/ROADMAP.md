# AM263P Zephyr Port — Roadmap

Living plan for the TI AM263P port and the motor-control platform built on it.
Source of truth for sequencing and status. Updated on **every commit** (see the
Changelog at the bottom; the convention is recorded in `/CLAUDE.md`).

**Detailed implementer hand-off:** see `porting/IMPLEMENTATION_PLAN.md` (verified
hardware facts, pin tables, existing code map, per-phase executable steps, and
the two-layer portability rules). This ROADMAP is the status tracker; the
Implementation Plan is the "what exactly to do" document.

## Next up (resume here)

**Phase 6 — motor peripherals (ePWM done; ADC next).**

Phases 1–5 are complete. The two-layer split is live: Layer 1 = this Zephyr
fork (SoC/board/dts/bindings/generic drivers); Layer 2 = `C:\am263-layer2`
(`framework/` + `apps/blink/`, own `west.yml`, `build.ps1`), which builds and
runs blink + shell + hardware LD7 PWM brightness on the ControlCARD.

**Phase 6 done so far:**
- ePWM driver `ti,am263x-epwm` (`drivers/pwm/pwm_ti_am263x.c`) — type-5 register
  map, period/duty/polarity, proven on hardware via LD7 brightness.
- Clock bring-up: `soc.c` enables the CONTROLSS source clock; the driver enables
  per-instance `EPWM_CLKSYNC`. EPWMCLK verified = 200 MHz (GEL + SDK).
- **ADC driver `ti,am263x-adc`** (`drivers/adc/adc_ti_am263x.c`) + binding + DT:
  register map audited against SDK `cslr_adc.h`; per-instance clock ungate,
  reset, TOP_CTRL ref buffer (`MASK_ANA_ISO` sequence), 12-bit single-ended.
  Software-trigger + INT1 poll aligned with SDK `adc_soc_software`. **Proven on
  hardware** (TMDSHSECDOCK pots → ADC0_AIN0/1; mid-travel raw ~3800 @ 1.8 V FS).
  Layer-2 `adc read` / `adc dump` shell commands.

**Phase 6 remaining:**
1. **PWM-synced control ISR** — ePWM SOCA/SOCB triggers ADC start-of-conversion;
   ADC EOC fires the control ISR. This is the deterministic control loop core.
2. Wire a small Layer-2 demo app (e.g. read a pot → set LD7 brightness) to prove
   the ePWM↔ADC↔ISR chain end to end.

After Phase 6: Phase 7 (dual-core AMP — sysbuild + IPC).

Resume context: read `/CLAUDE.md` (rules + verified-facts ledger — clock/RTI
*and* the new ePWM sections) and the latest changelog below. Hardware docs in
`vendor-docs/`; register maps also in the MCU+ SDK at
`C:\ti\mcu_plus_sdk_am263px_26_00_00_06\source\drivers`. In-tree smoke test:
`.\build.ps1` (defaults to `samples\hello_world`). Real apps:
`cd C:\am263-layer2 ; .\build.ps1`. Debug over XDS110 (Core_R5_0 only),
console 115200 8N1.

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
- [x] **Phase 5 — single-core app skeleton (Layer 2)**: framework extracted to
      the out-of-tree Layer-2 workspace (`C:\am263-layer2`: `framework/` +
      `apps/blink/`, own `west.yml` importing this fork, `build.ps1`). Builds and
      runs the blink + shell (and now hardware PWM brightness) on the ControlCARD.
      In-tree `samples/am263_app` reduced to a README pointer; the two-layer
      contract holds (no app logic in this tree).
- [~] **Phase 6 — motor peripherals**: ePWM driver → ADC driver → PWM-synced
      control ISR (custom drivers; no upstream Zephyr equivalents yet). **ePWM
      and ADC (software-trigger poll) done + proven on hardware**; next: ePWM
      SOCA → ADC EOC ISR chain + pot→PWM demo.
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

### 2026-06-01 — feat(am263p): Phase 6 ADC driver proven on hardware (dock pots)
- `drivers/adc/adc_ti_am263x.c`: full bring-up path — `CONTROLSS_CTRL` per-ADC
  clock ungate + reset, TOP_CTRL ref buffer with `MASK_ANA_ISO` (SDK soc.c),
  12-bit single-ended `ADCCTL2`, ACQPS = sampleWindow−1. INT1 source/enable in
  `channel_setup()` (SDK `adc_soc_software`); `read()` polls INT1 only.
- Board: `&adc0 { status = "okay"; }`, `dock_pot0/1` voltage-divider nodes +
  aliases (SPRUJ73 dock → ADC0_AIN0/1).
- `dts/bindings/adc/ti,am263x-adc.yaml`, `am263p4.dtsi` adc0/1 nodes, Kconfig/CMake.
- `CLAUDE.md`: ADC ledger (VREF ~1.8 V vs 3.3 V pots, jumpers SW9/J65/J66).
- `vendor-docs/spruj73-dock.pdf` added. `.gitignore`: exclude `mcps/`, `.claude/`,
  `terminals/` local artifacts.
- Hardware: `adc read 0/1` tracks pot (~3800 mid-travel with 1.8 V FS); no
  ETIMEDOUT. Layer-2 shell in separate `am263-layer2` commit.
- Phase touched: Phase 6 (ADC software path proven; PWM-synced ISR next).

### 2026-06-02 — feat(am263p): Phase 6 ADC driver skeleton (software trigger + poll)
- Added `dts/bindings/adc/ti,am263x-adc.yaml` (compatible "ti,am263x-adc",
  two-reg (cfg+result), optional pinctrl, io-channel-cells=1).
- `dts/arm/ti/am263p4.dtsi`: added disabled `adc0`/`adc1` nodes with verified
  bases (SDK cslr_soc_baseaddress.h: ADC0 cfg 0x502C0000 / result 0x50100000).
  IRQ via CONTROLSS XBAR left for later (polling first). Updated memory-map.md
  cross-ref.
- New `drivers/adc/adc_ti_am263x.c`: minimal Zephyr ADC driver (channel_setup
  with SW-only SOC, adc_read polling on ADCINTFLG after forceSOC, 12-bit read
  from result window). Uses CONTROLSS clock (already up). Cited CSL + TRM §7.5.2.
- `drivers/adc/{Kconfig,CMakeLists.txt}`: wired CONFIG_ADC_TI_AM263X +
  source (depends on DT_HAS_TI_AM263X_ADC_ENABLED).
- ROADMAP: updated Phase 6 "done so far" + new changelog entry. No board enable
  or Layer-2 shell cmd yet (next: enable on tmdscncd263p + test read; refbuf if
  needed from SDK soc.c).
- Phase touched: Phase 6 (ADC bring-up start; ePWM remains proven).

### 2026-06-01 — fix(am263p): audit + fix the AM263x ADC driver (Phase 6 ADC bring-up)
- Audited `drivers/adc/adc_ti_am263x.c` against MCU+ SDK `adc/v2/cslr_adc.h` +
  `soc.c` (same discipline as the ePWM driver). It had the same class of bugs:
  - Wrong offsets: `ADCSOCFRC1` 0x0E→**0x1A**, `ADCSOC0CTL` 0x10→**0x20**,
    `ADCSOCFLG1` 0x0C→**0x18** (0x0E/0x10 are actually ADCINTSEL1N2/3N4).
  - Wrong `ADCSOCxCTL` field positions: now `ACQPS[8:0]`, `CHSEL[19:15]`,
    `TRIGSEL[26:20]` (were [3:0]/@8/@16).
  - Missing `ADCINTSEL1N2` config → `ADCINTFLG` never set → every read timed out.
    Now `read()` maps INT1SEL=SOC + INT1E before forcing the SW SOC.
  - Missing reference buffer → no valid conversion. Added `adc_ti_enable_reference`
    (TOP_CTRL `0x50D80000`, KICK-locked: REFBUF0_CTRL=0x7 + REF_COMP_CTRL),
    mirroring SDK `SOC_enableAdcReference`, + a 500 µs power-up settling delay.
  - Channel map simplified to SOC n = AIN n (`channel_id`), avoiding a
    `CONFIG_ADC_CONFIGURABLE_INPUTS` dependency.
- Verified-correct and kept: bases (cfg 0x502C0000 / result 0x50100000), ADCCTL1/2
  offsets+bits, PRESCALE=6 (/4), result stride 2 / 12-bit, ADC clock (CONTROLSS
  source + per-ADC gate ungated at reset).
- Layer-2: `apps/blink/cmd_adc.c` adds `adc read <ch>`; prj.conf enables
  `CONFIG_ADC` + `CONFIG_ADC_TI_AM263X`.
- Board DTS: added required `output-ohms`/`full-ohms` (1:1) to the dock_pot
  voltage-divider nodes so the DT build passes (pot wiper is a direct input).
- CLAUDE.md ADC ledger corrected (the prior offsets/fields there were the wrong
  ones that caused the bug). Not yet hardware-verified.

### 2026-06-01 — chore(am263p): close out Phase 5 (Layer-2 extraction complete)
- Phase 5 flipped to `[x]`. The Layer-2 workspace (`C:\am263-layer2`) is the live
  home for app logic and builds/runs blink + shell + LD7 PWM on hardware.
- Removed the duplicated in-tree app code under `samples/am263_app/` (framework,
  apps/blink, main.c, CMakeLists/Kconfig/prj.conf) — `git rm`. Left only a
  `README.md` pointer to the Layer-2 workspace, honouring the two-layer contract
  (no app logic in the Zephyr fork).
- `build.ps1`: default `-Sample` `samples\am263_app` → `samples\hello_world`
  (the in-tree smoke test), since the app sample no longer exists in-tree.
- Rewrote the ROADMAP "Next up" to target Phase 6 (ADC + PWM-synced control ISR;
  ePWM already done). Marked Phase 6 `[~]` with the ePWM driver done.
- No firmware/behaviour change; tree/process cleanup only.

### 2026-06-01 — fix(am263p): LEDs are active-HIGH, not active-LOW (LD6/LD7 now alternate)
- After brightness was fixed, LD6 and LD7 blinked *in sync* instead of
  alternating, even though the task drives them in opposite slots. That can only
  happen if LD6's real polarity is flipped from its `GPIO_ACTIVE_LOW` decl.
- Conclusion (consistent with LD7's NORMAL ePWM polarity): the ControlCARD user
  LEDs are **active-HIGH** — the long-standing "active-LOW" from SPRUJ86/datasheet
  was never eyeball-confirmed (blink alternation hides absolute polarity).
- `boards/ti/tmdscncd263p/...dts`: LD6 `GPIO_ACTIVE_LOW` → `GPIO_ACTIVE_HIGH`.
  Updated the CLAUDE.md LED ledger to record active-HIGH and why.

### 2026-06-01 — fix(am263p): LD7 ePWM polarity is NORMAL not inverted (brightness was reversed)
- After the 0%-duty fix, `blink pwmdump` confirmed the full chain healthy
  (TBCTR counting, AQCTLB=0x0102, CMPB tracking duty). A brightness sweep then
  showed brightness was *reversed* (100=off, 0=full) and LD7 lit during LD6's
  slot — i.e. `PWM_POLARITY_INVERTED` was the wrong choice.
- Measured CMP→LED behavior: CMP=0 → off, CMP=max → full on. So LD7 via EPWM7_B
  is brightness-proportional with NORMAL polarity (pin-high = brighter) — it
  behaves active-HIGH under ePWM, despite being active-LOW as GPIO. Recorded as
  a verified fact in CLAUDE.md (evidence over the GPIO-derived assumption).
- `boards/ti/tmdscncd263p/...dts`: LD7 `pwms` flag INVERTED → NORMAL. This fixes
  both symptoms at once: brightness now 0=off…100=full, and duty 0 (LD6's slot)
  → CMP=0 → LD7 truly off. Softened the driver's polarity comment (polarity is a
  board/DT fact, not assumed in the driver).

### 2026-06-01 — fix(am263p): ePWM 0%-duty corner case (LD7 stuck on at inverted polarity)
- Hardware `blink pwmdump` proved the clock chain is now fully working:
  TBCTR COUNTING (6761→16905), CTRMODE=up, FREE_SOFT=2, EPWM_CLKSYNC bit7=1,
  CONTROLSS_STATUS=0x04, LD7 pad mux=0. The only wrong value was `CMPB=0`.
- Root cause: the driver implemented inverted polarity by swapping the AQ
  actions (ZRO=clear, CBU=set). At CMP=0 that leaves the pin stuck LOW, which
  on the active-LOW LED = permanently ON. The blink task drives 0% duty during
  LD7's off-slot, so LD7 was on in both slots → "always on".
- Fix `drivers/pwm/pwm_ti_am263x.c`: always use the well-behaved action set
  (ZRO=set high, compare=clear low) and handle polarity by complementing the
  compare value (`CMP = period - pulse` when inverted) instead of swapping
  actions. This is clean at both extremes: CMP=0 → 0% high, CMP>TBPRD → 100%
  high. After this, AQCTLB reads 0x0102 (was 0x0201) and 0% duty truly turns
  LD7 off.
- Added the `blink pwmdump` shell command (Layer-2 `cmd_blink.c`) that reads
  TBCTR twice + the key ePWM/clock registers — the tool that localized this.

### 2026-06-01 — fix(am263p): enable CONTROLSS source clock in soc.c (ePWM counter was frozen → LD7 stuck on)
- Symptom on hardware: `blink brightness` did nothing; LD7 sat permanently ON.
  With inverted polarity that is the exact signature of a frozen time-base
  counter (zero-action drives the active-LOW pin low = LED on, and the compare
  is never reached).
- Root cause: the ePWM needs TWO clock enables, and we only did one.
  `EPWM_CLKSYNC` (per-instance, in the driver) was set, but the **CONTROLSS
  subsystem source clock** — which actually feeds the counters — was never
  enabled by our firmware. The CCS GEL enables it (`Program_CONTROLSS_Clocks`,
  called from `Configure_All_Peripheral_Clks`), but a "Restart"/GEL-less boot
  leaves it gated. RTI had the same class of bug and was already fixed the same
  way; ePWM was missed.
- Verified the exact register sequence against BOTH the GEL and the SDK
  (`soc_rcm.c` `SOC_RcmPeripheralId_CONTROLSS_PLL`, `gControlssPllClkSrcValMap`,
  `cslr_mss_rcm.h`): MSS_RCM `CONTROLSS_PLL_CLK_DIV_VAL=0`, `SRC_SEL=0x222`
  (DPLL_CORE_HSDIV0_CLKOUT2), `CLK_GATE=0` (ungate), poll `CLK_STATUS`==0x4.
- `soc/ti/am263x/soc.c`: added `am263p_enable_controlss_clock()`, called from
  `soc_early_init_hook()` right after the RTI clock enable (bounded status poll
  so a clock fault can't hang boot). Recorded both clock enables in CLAUDE.md.
- Layer-2 `apps/blink/main.c`: boot banner now prints `Built: __DATE__ __TIME__`
  and the feature under test (LD7 hardware ePWM brightness) so the loaded ELF is
  identifiable.

### 2026-06-01 — docs(am263p): verify EPWMCLK = 200 MHz from GEL + SDK (was assumed 100 MHz)
- Chased the ePWM clock through the CCS GELs and MCU+ SDK instead of leaving it
  assumed:
  - GEL `Program_CONTROLSS_Clocks()` sets the CONTROLSS/ePWM subsystem clock to
    **400 MHz** (`SRC_SEL=0x222`, `DIV=/1`, STATUS-polled).
  - SDK `etpwm.h`: "EPWMCLK is a scaled version of SYSCLK. At reset EPWMCLK is
    half SYSCLK" → 400 / 2 = **200 MHz**.
  - SDK example `epwm_xcmp_dma.c`: `EPWM_TBCLK_FREQUENCY_IN_HZ = 200000000U`.
- `drivers/pwm/pwm_ti_am263x.c`: `get_cycles_per_sec` 100 MHz → **200 MHz**
  (removed the UNVERIFIED tag, added the citations). TBCLK = EPWMCLK (no prescale).
- Updated the DTS/task carrier comments (250 us x 200 MHz = 50000 < 65535, still
  fits 16-bit TBPRD) and the CLAUDE.md ePWM ledger. No functional change to the
  brightness *ratio* (clock-independent); only the carrier frequency is now correct.

### 2026-06-01 — fix(am263p): ePWM driver — correct type-5 register map + TB clock enable (LD7 brightness)
- Root-caused why `blink brightness` did nothing. Verified from primary sources
  (SPRSP81 Table 5-1, TRM SPRUJ55 §7.5.6, MCU+ SDK CSL headers) — recorded in
  the CLAUDE.md "Verified ePWM facts" ledger:
  - The driver used **legacy type-0** register offsets; AM263P is **type-5**.
    Real offsets: `TBPRD 0xC6`, `CMPA 0xD4` (32-bit, value `<<16`),
    `CMPB 0xD8`, `AQCTLA 0x80`, `AQCTLB 0x84`.
  - The **time-base clock was never enabled** — needs per-instance
    `CONTROLSS_CTRL.EPWM_CLKSYNC` bit (base `0x502F0000`+`0x10`) behind a KICK
    lock. Counter was frozen → no output.
  - AQ action constants were inverted (1=clear, 2=set); period was off-by-one.
  - **LD6 physically cannot do ePWM** (pad 0x53100058 has no ePWM mux option);
    LD7's ePWM mux mode was a wrong placeholder (`0x1`; correct = mode 0 = `0x100`).
- `drivers/pwm/pwm_ti_am263x.c`: rewritten — correct offsets, 32-bit CMPA/CMPB,
  AQ encoding, `PWM_POLARITY_INVERTED` support (active-LOW LEDs), `TBPRD=period-1`,
  period-overflow guard, and `EPWM_CLKSYNC` enable (KICK unlock per SDK
  `SOC_setEpwmTbClk`). Instance index derived from the DT reg base.
- `dts/bindings/pwm/ti,am263x-epwm.yaml` + `dts/arm/ti/am263p4.dtsi`:
  `#pwm-cells` 2 → 3 (add `flags`) so polarity reaches the driver.
- `boards/ti/tmdscncd263p/...dts`: LD7 pwm pinmux `0x1` → `0x100` (EPWM7_B mode 0);
  removed the impossible LD6 ePWM pinctrl + `&epwm0` enable; LD6 stays GPIO-only;
  `pwmleds` now LD7-only with `PWM_POLARITY_INVERTED` @ 4 kHz; disabled `&gpio1`
  (its only consumer LD7 is now ePWM-owned); dropped the `led1`/`led_ld7` GPIO node.
- Layer-2 `C:\am263-layer2\apps\blink\task_blink.c`: LD6 via GPIO, LD7 via PWM
  (`brightness` = LD7 duty). Phase 6 groundwork (first real ePWM driver).
- Not yet hardware-verified — pending build + flash on the ControlCARD.

### 2026-06-02 — feat(am263p): Phase 5 extraction progress — workspace now testable
- Improved `C:\am263-layer2` for immediate testing:
  - `build.ps1` now automatically activates the same venv (`C:\zephyr-ti\.venv-zephyr`) used by the main tree. This solves the "west not found / no venv" problem.
  - Added `build.ps1` helper + per-app README.
  - Added `blink brightness <0-100>` command (software PWM in the task for quick LED dimming testing).
  - Real hardware ePWM driver skeleton + pinctrl started in Layer-1 (`C:\zephyr-ti`).
  - Better `west.yml` that points directly at your local `C:\zephyr-ti`
  - `apps/blink` can now be built directly with `west build -b tmdscncd263p/am263p4/r5f0_0 apps/blink`
  - Added root `Kconfig` and `prj.conf`
  - Updated blink `CMakeLists.txt` to support both included and standalone builds
  - Detailed test instructions added to the Layer-2 README.md
- Goal achieved: you can now build + flash the extracted blink app and test the shell commands on the ControlCARD.
- In-tree `samples/am263_app` is now clearly marked as legacy reference.

### 2026-06-02 — feat(am263p): start Phase 5 Layer-2 extraction (out-of-tree workspace scaffolding)
- Created initial out-of-tree Layer-2 workspace at `C:\am263-layer2` (sibling to main Zephyr tree).
- Bootstrapped:
  - `west.yml` (placeholder for pulling Layer-1 Zephyr)
  - `framework/app.[ch]` (registration layer)
  - `apps/blink/` (full app with task + shell commands, moved from in-tree reference)
  - Supporting `Kconfig`, `CMakeLists.txt`, `prj.conf`, and example `src/main.c`
- This begins the strict separation required by the two-layer model.
- In-tree `samples/am263_app` will be reduced to a thin reference/shim going forward.
- See new `C:\am263-layer2\README.md` for workspace layout and build notes.
- ROADMAP "Next up" section already points at the remaining extraction checklist.

### 2026-06-02 — docs(am263p): clock root cause chase — RTI 100 MHz despite GEL/TRM "200 MHz SYS_CLK via 0x222"
- Fetched + analyzed official TI CCS GELs from the path the user provided (`C:\ti\ccs2051\ccs\ccs_base\emulation\gel\AM263Px\`).
  - `AM263Px.gel` `OnTargetConnect()` → `Configure_Plls_R5F_400_SYS_200_Clocks()` + `Configure_All_Peripheral_Clks()` (enables RTI0-3).
  - `AM263Px_PLL\AM263Px_PLL.gel`: `Program_Core_PLL()` + `Program_SYS_CLK_DIVBY2()` (0x111) + `Program_R5F_SYS_CLK_SRC()` (0x222) → prints "R5F=400MHz and SYS_CLK=200MHz".
  - `AM263Px_PLL\AM263Px_Periheral_Clocks.gel` `Program_RTI0_Clocks()` (and siblings): exact same writes our `soc.c` uses — `MSS_RCM_RTI0_CLK_SRC_SEL = 0x222; ..._DIV_VAL = 0x000;` + `GEL_TextOut("RTI0 Clock Enabled (200MHz)\n")`.
- TRM SPRUJ55 primary sources (pdftotext targeted extracts):
  - §6.4.8.2.1 "RTI CLOCK (FREQ = 200MHz)": documents precisely `MSS_RCM.RTIx_CLK_DIV_VAL = 0x000` + `SRC_SEL = 0x222` "to select SYS_CLK as its source".
  - Table 4-56 (§4.21 RTI Integration): **FCLK (RTI_CLK to the FRC counters)** selectable from SYS_CLK (PLL_CORE HSDIV0 @ 200 MHz intended) vs ICLK (VBUSP @ 200 MHz). Other FCLK options include XTAL 25 MHz, EXT_REFCLK 100 MHz, etc.
- Key finding: the exact configuration GEL + our `soc.c:am263p_enable_rti_clock()` perform is the documented one for 200 MHz RTI FCLK. Yet direct wall-clock measurement (FRC0 deltas vs stopwatch) shows 100 MHz effective on the TMDSCNCD263P. The "SYS_CLK" tap visible to the MSS_RCM RTI0 mux (or the FCLK path downstream of it) runs at 100 MHz under this PLL + TOP_RCM DIVBY2 setup, while GEL labels the R5/SYS tree "200 MHz".
- Result: the 100 MHz `SYS_CLOCK_HW_CYCLES_PER_SEC` value (Phase 4 fix) is correct and required for wall time on this board. Open thread from the prior "RTI counter is 100 MHz" entry is now substantially narrowed (GEL + TRM cited; no value change needed). Recorded in CLAUDE.md verified facts ledger.

### 2026-05-31 — chore(am263p): blink `interval` command + CLAUDE.md working-method notes
- `samples/am263_app`: renamed the blink control `rate`/`period` → `interval`,
  where the value is the per-LED on-time in ms (`interval 1000` = each LED lit
  1 s, LEDs swap every 1 s). Clearer than the earlier half-period naming that
  made users suspect a clock bug.
- `CLAUDE.md`: added a "Working method" section (evidence-over-theory,
  read-before-edit, primary sources, one verified change at a time, write down
  what you learn) — portable guidance for any agent working this repo.

### 2026-05-31 — fix(am263p): RTI counter is 100 MHz, not 200 — timer now wall-clock accurate
- `soc/ti/am263x/Kconfig.defconfig`: `SYS_CLOCK_HW_CYCLES_PER_SEC` 200M → 100M.
- Measured directly on the ControlCARD: over a 10 s stopwatch, `k_uptime_get()`
  advanced ~5.14 s, i.e. the RTI FRC0 counter runs at half the assumed rate.
  Everything timed (`k_msleep`, uptime) was 2× slow in wall-clock terms while
  staying self-consistent — which is why the earlier "uptime advances" check
  didn't catch it. UART console is clean, so this is the RTI source clock
  specifically, not a global 2× error.
- UNVERIFIED why it is /2 (SYS_CLK/2 vs SYS_CLK actually 100 MHz vs the
  `RTI0_CLK_DIV_VAL=0` write not being /1) — to confirm against TRM SPRUJ55
  MSS_RCM RTI clock mux/divider + the GEL.

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

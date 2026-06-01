# CLAUDE.md — working rules for this repo

Zephyr RTOS bring-up port for the **TI AM263P** running on the **TMDSCNCD263P
ControlCARD** (not the LaunchPad), R5FSS0 core0.

## Rule 1 — Do not assume hardware facts. Ever.

Register addresses, pin/ball mappings, pinmux mux-mode values, bit-field
layouts, clock frequencies, memory map, peripheral instance bases, IRQ numbers,
LED/button wiring — **none of these may be guessed, inferred "by analogy," or
carried over from another SoC/board.** A wrong register poke wastes hours and
can fault the core.

If a fact is needed and not already **verified from an authoritative source**:

1. **Stop.** Do not write the value as a placeholder-that-looks-real or a
   "best guess."
2. **Narrow it down precisely** and ask the user for it: name the exact
   document and the exact section / table / register you need (e.g. "AM263Px
   TRM, IOMUX chapter — the PADCONFIG register bit-field table" or "datasheet
   SPRSP81 pin-mux table — which ball has GPIO22 as a mux option and what mode
   number"). One specific ask, not "send me the docs."
3. The user downloads it into **`vendor-docs/`** and tells you the filename.
   Read it from there (Claude can read PDFs directly).

Acceptable sources, in order: (a) `vendor-docs/` PDFs, (b) the TI MCU+ SDK
source / official TI docs fetched online **with the source cited**, (c) values
the user states directly. Mark anything not yet from one of these as
**UNVERIFIED** in code comments and call it out — do not let it pass silently.

## Rule 2 — Vendor docs live in `vendor-docs/`

Check there first. See `vendor-docs/README.md` for what's present and the
"wanted" list. Never dump reference PDFs into Zephyr's own `docs/` tree.

## Build / flash environment (Windows)

- Workspace quirk: `.west/` lives at **`C:\`** root, so west/CMake can resolve
  `WEST_TOPDIR=C:\` and fail with "Could not find Zephyr package."
  `build.ps1` works around this by pinning `ZEPHYR_BASE=C:\zephyr-ti`.
- Build: `.\build.ps1` (board `tmdscncd263p/am263p4/r5f0_0`, sample
  `samples/hello_world`; `-Board`/`-Sample` override, `-Pristine` forces a clean
  build). Zephyr SDK 1.0.1 at `C:\Users\abhin\zephyr-sdk-1.0.1`.
- ELF: `build/zephyr/zephyr.elf`. It is a **RAM load** (TCM + OCRAM
  @0x70040000), loaded over JTAG via XDS110 — not flashed.
- Debug: online CCS / CCS Cloud, XDS110 probe. Connect **Core_R5_0 only**.
  Load Program → Restart → Resume. The `-215` "semaphore time-out" error = a
  stale `DSLite` debug server owns the probe; kill it and replug XDS110.

## Verified hardware facts (with sources — extend, don't guess)

- GPIO0 controller base `0x52000000`; GPIO1 `0x52001000`. (MCU+ SDK
  `CSL_GPIO0_U_BASE`.)
- GPIO register groups are Davinci-style banked, 32 pins per register
  (`pin>>5` = group index, `pin&31` = bit): group0 (pins 0-31) DIR/OUT/SET/CLR
  at `0x10/0x14/0x18/0x1C`; group1 (pins 32-63) at `0x38/0x3C/0x40/0x44`.
  (MCU+ SDK `gpio.h`: `GPIO_PINS_PER_REG_SHIFT=5`.)
- IOMUX pad-config base `0x53100000`; each pad's CFG reg = base + offset.
  Pad-cfg bits: [3:0]=muxmode (GPIO is **mux mode 7** on every pad checked),
  [7:4]=force in/out enable/disable, [8]=pull-disable, [9]=pull up/down,
  [17:16]=GPIO core owner (0=R5SS0-core0). Reset default `0x05F7` = mode7 +
  force-out-disable; GPIO-output value = `0x107` (mode7, pull off, no force,
  so the GPIO DIR reg controls output). (datasheet SPRSP81 Table 5-1 +
  MCU+ SDK `pinmux.h`.)
- **ControlCARD user LEDs are active-HIGH (corrected 2026-06 from hardware).**
  The pin drives the LED on when HIGH. This overturns the earlier "active-LOW"
  reading of SPRUJ86 Table 2-12 / datasheet — that was never confirmed by eye
  (blink alternation looks the same either polarity). Hardware proof: LD7 via
  ePWM is brightness-correct only with `PWM_POLARITY_NORMAL` (pin-high = bright),
  and LD6/LD7 blinked *in sync* while LD6 was declared `GPIO_ACTIVE_LOW` —
  flipping LD6 to `GPIO_ACTIVE_HIGH` made them alternate as intended. DT now
  uses `GPIO_ACTIVE_HIGH` (LD6) / `PWM_POLARITY_NORMAL` (LD7).
- LED pad/signal locations (SPRUJ86 Table 2-12 + datasheet SPRSP81 Table 5-1;
  pad/mux still authoritative, only the active-level was wrong):
  - **USER_LED1 / LD7 = GPIO58**, pad EPWM7_B, CFG reg `0x531000E8`, mode 7.
  - **USER_LED0 / LD6 = GPIO22**, pad LIN2_TXD (ball A8), CFG reg `0x53100058`,
    mode 7. (SPRUJ86 Table 2-12 mislabels this pad "EPWM7_B" — a typo.)
  - Datasheet adjacency check: LIN2_RXD (B8, `0x53100054`) mode 7 = GPIO21.

### Verified clock / RTI facts (from GEL + TRM chase, 2026-06)
- Official CCS GELs (from `C:\ti\ccs2051\ccs\ccs_base\emulation\gel\AM263Px\`):
  - `AM263Px.gel` `OnTargetConnect()` calls `Configure_Plls_R5F_400_SYS_200_Clocks()` then `Configure_All_Peripheral_Clks()`.
  - `AM263Px_PLL\AM263Px_PLL.gel`: `Program_Core_PLL()` (HSDIV0=0x4 → 400 MHz comment from 2 GHz VCO; HSDIV1=0x3 → 500 MHz), `Program_SYS_CLK_DIVBY2()` writes `MSS_TOP_RCM_SYS_CLK_DIV_VAL = 0x111` ("/2"), `Program_R5F_SYS_CLK_SRC()` writes 0x222 (selects DPLL_CORE_HSDIV0_CLKOUT0) and prints "R5F=400MHz and SYS_CLK=200MHz".
  - `AM263Px_PLL\AM263Px_Periheral_Clocks.gel` `Program_RTI0_Clocks()` (and RTI1-3/WDT equivalents): exactly
    `Write_MMR(MSS_RCM_RTI0_CLK_SRC_SEL, 0x222); Write_MMR(MSS_RCM_RTI0_CLK_DIV_VAL, 0x000);` then polls STATUS==0x4 and prints "RTI0 Clock Enabled (200MHz)".
  - Matches what `soc/ti/am263x/soc.c:am263p_enable_rti_clock()` does (for GEL-less/SBL boots).
- TRM SPRUJ55 (primary):
  - §6.4.8.2.1 "RTI CLOCK (FREQ = 200MHz)": explicitly documents `MSS_RCM.RTIx_CLK_DIV_VAL.CLKDIV = 0x000` + `RTIx_CLK_SRC_SEL.CLKSRCSEL = 0x222` "to select SYS_CLK as its source".
  - Table 4-56 (RTI Clocks, §4.21): **RTI0_FCLK (RTI_CLK)** — the clock that feeds the FRC0/UC0 counters — can be sourced from SYS_CLK (PLL_CORE HSDIV0_CLKOUT0 @ 200 MHz intended) among other options (default often XTAL 25 MHz or EXT_REFCLK 100 MHz). Separate **RTI0_ICLK (VBUSP interface @ 200 MHz)** from the same SYS_CLK tap.
  - 0x222 is the standard redundant GCM select encoding for the "SYS_CLK" input across TOP_RCM and MSS_RCM muxes.
- Observed on TMDSCNCD263P ControlCARD (GEL run + our identical re-init): FRC0 advances at **100 MHz** effective (k_uptime_get vs stopwatch). UART console clean. DT already records GEL-measured 160 MHz UART clock on this board.
- Result: documented 200 MHz "SYS_CLK → RTI via 0x222/DIV=0" path is active and matches GEL/TRM, but the effective FCLK rate to the timer counters is 100 MHz on this board/setup. The SYS_CLK tap seen by the MSS_RCM RTI mux (or downstream FCLK distribution) delivers half the rate that the TOP_RCM R5/SYS tree and GEL "200 MHz" labels assume. (FCLK vs ICLK distinction in Table 4-56 allows the counters to see 100 MHz while other logic sees the full rate.)
- Therefore `CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC = 100000000` (set in Phase 4) is the correct value for wall-clock time on this ControlCARD. No change to the 100 MHz setting required.

### Verified ePWM facts (from MCU+ SDK CSL + SPRSP81/SPRUJ55, 2026-06)
- **This is "etpwm" type-5, NOT legacy C2000 type-0.** TRM SPRUJ55 §7.5.6:
  "applicable for ePWM type 5". The register map differs from the classic
  type-0 offsets — anything using `0x0A/0x0E/0x16` is wrong on this SoC.
- ePWM register offsets (MCU+ SDK `drivers/epwm/v1/cslr_etpwm.h`):
  `TBCTL 0x00` (16b), `TBCTR 0x08` (16b), `AQCTLA 0x80` (16b),
  `AQCTLB 0x84` (16b), `TBPRD 0xC6` (16b), `CMPA 0xD4` (**32b**),
  `CMPB 0xD8` (**32b**). CMPA/CMPB layout: `[31:16]=CMPx` (integer compare),
  `[15:0]=CMPxHR` (HRPWM fraction) — write duty as `value << 16`.
- AQ action encoding (TRM §7.5.6.6 + cslr_etpwm.h field shifts): per field
  `0=disable, 1=clear(low), 2=set(high), 3=toggle`. Fields: `ZRO[1:0]`,
  `PRD[3:2]`, `CAU[5:4]`, `CBU[9:8]`. Use the well-behaved single-edge config
  **ZRO=set + CAU/CBU=clear for BOTH polarities** (AQCTLx = 0x12 for chan A,
  0x102 for chan B). Implement inverted polarity by complementing the compare
  (`CMP = period − pulse`), **not** by swapping to ZRO=clear. Reason: ZRO=clear
  with CMP=0 leaves the pin stuck low (active-LOW LED = permanently ON — the
  observed LD7 bug). With ZRO=set: CMP=0 → 0% high, CMP>TBPRD → 100% high, both
  clean.
- Up-count period: `T_pwm = (TBPRD+1) × TBCLK` (TRM §7.5.6.4.3) → program
  `TBPRD = period_cycles − 1`. TBPRD is 16-bit (period ≤ 0x10000 cycles).
- **ePWM needs TWO clock enables; both are gated after a GEL-less boot or a
  CCS "Restart". The GEL does both; our code must too.**
  1. **CONTROLSS subsystem source clock** (feeds every ePWM/eCAP counter) —
     in **MSS_RCM** (`0x53208000`, KICK-locked with `0x68EF3490/0xD172BC5A`):
     `CONTROLSS_PLL_CLK_DIV_VAL @0x260 = 0x000`,
     `..._CLK_SRC_SEL @0x160 = 0x222` (DPLL_CORE_HSDIV0_CLKOUT2),
     `..._CLK_GATE @0x360 = 0x000` (ungate; `0x7` = gated), poll
     `..._CLK_STATUS @0x460` low byte == `0x4` (CLKINUSE). Done in
     `soc.c:am263p_enable_controlss_clock()`. Mirrors GEL
     `Program_CONTROLSS_Clocks()` + SDK `soc_rcm.c`
     (`SOC_RcmPeripheralId_CONTROLSS_PLL`). **Without this the time-base
     counter is frozen → with inverted polarity the LED sits ON permanently.**
  2. **Per-instance time-base sync** — `CONTROLSS_CTRL.EPWM_CLKSYNC`
     (base `0x502F0000`, offset `0x10`, **one bit per instance**, EPWM7 = bit 7).
     This partition is KICK-locked with the *other* key pair: unlock
     `LOCK0_KICK0 @0x1008 = 0x01234567`, `LOCK0_KICK1 @0x100C = 0x0FEDCBA8`
     (SDK `soc.h` `KICK*_UNLOCK_VAL`; same values the GEL uses), set the bit,
     relock. Done in the PWM driver init. Mirrors SDK `SOC_setEpwmTbClk()`.
     TRM §7.5.6.4.3.2 documents the set-0 → configure → set-1 sequence.
  (CONTROLSS_CTRL @0x502F0000 is reachable: the R5 MPU maps a 2 GB "Device"
  region at 0x0, see `arm_mpu_regions.c`.)
- EPWM instance bases: `EPWM0 = 0x50000000`, stride `0x1000`
  (`EPWM7 = 0x50007000`). Instance index = `(base − 0x50000000) / 0x1000`.
- **LED pad routing (SPRSP81 Table 5-1, authoritative):**
  - **LD7 (pad 0x531000e8)**: `EPWM7_B` = **mux mode 0** (also GPIO58=mode7,
    EPWM5_B=mode10). Pad value for PWM output = `0x100` (func_sel 0 +
    pull-disable[8]). So LD7 CAN do hardware PWM = EPWM7 channel B (channel 1).
  - **LD6 (pad 0x53100058, LIN2_TXD ball A8)**: mux options are ONLY
    LIN2_TXD(0)/UART2_TXD(1)/SPI2_D1(2)/GPIO22(7) — **no ePWM at all.** LD6 is
    GPIO-only; it physically cannot be brightness-controlled.
- **LD7 ePWM polarity = NORMAL (measured).** Driven via EPWM7_B, LD7 is
  brightness-proportional with `PWM_POLARITY_NORMAL` (CMP=pulse: 100% = full on,
  0% = off, mid = dim). So this pin behaves **active-HIGH under ePWM**, even
  though LD6/LD7 are active-LOW as GPIO. Confirmed by a `blink brightness` sweep
  + `blink pwmdump` on hardware (the inverted flag gave reversed brightness and
  lit LD7 during its off-slot). Don't "correct" this to INVERTED from the GPIO
  active-LOW fact — the ePWM path measured opposite.
- ePWM AQ coincidence at CMP=0: on this silicon, with ZRO=set + CBU=clear,
  CMP=0 resolves to **0% high** (compare-up wins) and CMP>TBPRD to 100% high —
  i.e. the well-behaved single-edge config, clean at both extremes.
- **EPWMCLK = 200 MHz** (so TBCLK = 200 MHz with no time-base prescale).
  Verified from TI sources (2026-06):
  - CCS GEL `AM263Px_Periheral_Clocks.gel` `Program_CONTROLSS_Clocks()`:
    `CONTROLSS_PLL_CLK_DIV_VAL=0x000`, `..._SRC_SEL=0x222`, polls STATUS==0x4,
    prints "CONTROLSS Clock Enabled (**400MHz**)" — this is the ePWM subsystem
    ("SYSCLK" in TRM Fig 7-179) clock.
  - MCU+ SDK `drivers/epwm/v1/etpwm.h`: "EPWMCLK is a scaled version of SYSCLK.
    **At reset EPWMCLK is half SYSCLK.**" → 400 / 2 = 200 MHz.
  - SDK example `epwm_xcmp_dma.c`: `EPWM_TBCLK_FREQUENCY_IN_HZ = 200000000U`.
  Caveat: the RTI "200 MHz" tap measured 100 MHz on this board (see RTI notes);
  no equivalent measurement exists for the separate CONTROLSS_PLL yet. Duty
  *ratio* is clock-independent, so brightness is correct regardless; only the
  carrier frequency would shift if a measurement later shows a /2 here too.

### Still to verify in the TRM (SPRUJ55) before broad BSP/devicetree work
- GPIO signal-number → controller-instance mapping: GPIO0 `0x52000000` vs
  GPIO1 `0x52001000`. Low numbers (GPIO22, GPIO58) are GPIO0; the GPIO0/GPIO1
  boundary (and how GPIO>127 like GPIO136/137 map) is NOT yet confirmed.
- The GPIO controller register struct offsets (currently from SDK `gpio.h`:
  banked, 32 pins/reg, group0 @0x10.., group1 @0x38..) — confirm against the
  TRM GPIO chapter register map.

## Workflow / roadmap upkeep

- **`porting/ROADMAP.md` is the source of truth** for sequencing and status.
  Read it to know what phase we're in and what's next.
- **Every commit must include a Changelog entry in `porting/ROADMAP.md`, in the
  same commit** — `### <date> — <commit subject>` plus bullets (what changed,
  which phase). No separate "changelog commit."
- When a phase completes, flip its `[ ]`→`[x]` checkbox and note it in the
  changelog.
- New AM263P/driver work follows the **two-layer model**: generic Layer-1
  support stays in-tree (`soc/`, `boards/`, `dts/`, `drivers/`, bindings);
  AMDC-style app logic stays in the out-of-tree Layer-2 workspace. Never bake
  app specifics into the Zephyr tree.

## Working method (applies to any agent on this repo)

These are the habits that make this port progress instead of thrashing. Follow
them; they matter more than raw cleverness.

- **Evidence over theory. Pull the artifact that already holds the answer.**
  Don't speculate about a bug when a file can decide it. Faulting PC → grep the
  `.map` for the symbol at that address. Wrong peripheral behavior → read the
  TRM register/section and compare to the live register dump. A guessed root
  cause is worth nothing; a quoted line/table/address is worth everything.
  (Examples: the RTI hang was solved by TRM §13.5.1.4.3 + the measured "FRC +1
  per ~21 s" matching `2^32 / 200 MHz`; the `0x70040000` crash was identified by
  finding `_vector_table` at that address in `zephyr.map`.)
- **Search and read before you edit.** Use grep/find to locate the *actual*
  code, read the whole function/struct, then change it. Never edit a file you
  haven't read. Say where you looked.
- **Primary sources only for hardware facts** (see Rule 1). `vendor-docs/`
  PDFs > cited TI SDK/online > values the user states. Quote the source; mark
  anything else **UNVERIFIED** and call it out.
- **One change at a time, verified on hardware before the next.** Make the
  minimal change, have the user build/flash and confirm, *then* commit and move
  on. Don't stack unverified changes.
- **Write down what you learn.** New verified facts go in the ledger above; new
  status/decisions go in `porting/ROADMAP.md`. The point is to never re-derive
  the same fact twice.
- **When blocked on a fact, narrow the ask to one specific document + table /
  register / section** and stop — don't fill the gap with a plausible-looking
  guess.

## Style

Match surrounding Zephyr conventions. Tabs in `.c`, devicetree idioms in
`.dts`. Don't commit `.claude/` or large binaries except curated PDFs in
`vendor-docs/`. Commit only when asked.

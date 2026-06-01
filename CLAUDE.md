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
- ControlCARD user LEDs, both active-LOW (SPRUJ86 Table 2-12 + datasheet
  SPRSP81 Table 5-1, datasheet authoritative):
  - **USER_LED1 / LD7 = GPIO58**, pad EPWM7_B, CFG reg `0x531000E8`, mode 7.
  - **USER_LED0 / LD6 = GPIO22**, pad LIN2_TXD (ball A8), CFG reg `0x53100058`,
    mode 7. (SPRUJ86 Table 2-12 mislabels this pad "EPWM7_B" — a typo.)
  - Datasheet adjacency check: LIN2_RXD (B8, `0x53100054`) mode 7 = GPIO21.

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

## Style

Match surrounding Zephyr conventions. Tabs in `.c`, devicetree idioms in
`.dts`. Don't commit `.claude/` or large binaries except curated PDFs in
`vendor-docs/`. Commit only when asked.

# AM263P application framework — moved out-of-tree (Layer 2)

The AMDC-style application framework (`app_/task_/cmd_` pattern, `apps_init()`
registration layer, Kconfig app-select, the `blink` example, and the Zephyr
shell wiring) **no longer lives in this tree.** Per the two-layer contract in
`CLAUDE.md`, application logic is kept out of the Zephyr fork.

It now lives in the separate **Layer-2 workspace** (out-of-tree), which consumes
this fork as Layer 1 via its own `west.yml`:

```
C:\am263-layer2\
  west.yml            # imports this Zephyr fork (file:///C:/zephyr-ti)
  framework/          # apps_init() + registration
  apps/blink/         # app_blink / task_blink / cmd_blink (+ hardware PWM brightness)
  build.ps1           # reuses the main tree's venv/SDK
```

Build the real apps from there:

```powershell
cd C:\am263-layer2
.\build.ps1 -Pristine          # board tmdscncd263p/am263p4/r5f0_0, app apps/blink
```

This Zephyr tree keeps only Layer-1 artifacts (SoC, board, devicetree,
bindings, generic drivers). For an in-tree smoke test use `samples/hello_world`
(`.\build.ps1 -Sample samples\hello_world`).

See `porting/ROADMAP.md` (Phase 5 = done; Phase 6 = motor peripherals) and
`CLAUDE.md` for the two-layer rules and the verified hardware-facts ledger.

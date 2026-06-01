# Debug Support

This directory contains convenience setups for different debuggers used during AM263P bring-up.

## CCS (Code Composer Studio) + XDS110

**Recommended for daily hardware debugging on the TMDSCNCD263P ControlCARD.**

See the dedicated folder:

- [debug/ccs/](ccs/) — contains:
  - `refresh-elf.ps1` (PowerShell version)
- `refresh-elf.cmd` (cmd.exe / Command Prompt version – no PowerShell required)
  - Starter `.ccxml` template
  - Full instructions in `README.md`

This gives you a clean place to keep your `.ccxml`, a reusable `.launch` file, and the latest `zephyr.elf` right next to each other.

## Current Debug Flow (ControlCARD + XDS110)

1. Build with `.\build.ps1`
2. Run `.\debug\ccs\refresh-elf.ps1`
3. In CCS: open your saved target configuration (or `.launch`)
4. Connect **only Core_R5_0**
5. Load Program → point at `debug/ccs/zephyr.elf`
6. Restart → Resume

Console appears on the XDS110 virtual COM port (115200 8N1).

See `porting/ROADMAP.md` for the current porting phase and what is being validated.

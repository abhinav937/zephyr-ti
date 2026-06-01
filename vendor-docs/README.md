# vendor-docs — TI AM263P hardware reference

Authoritative TI documents for this port. **Claude must consult these (and ask
for missing ones) instead of guessing hardware facts** — see `/CLAUDE.md`.

## Present

| File | Doc # | What it covers |
|------|-------|----------------|
| `am263p-trm-spruj55.pdf` | SPRUJ55D | **AM263P Technical Reference Manual** — full register descriptions: IOMUX pad-config bit fields, GPIO controller register map, clocks/PLLs, RTI timer, UART |
| `am263p-datasheet-sprsp81.pdf` | SPRSP81 | AM263Px datasheet: pin attributes/mux table, IOMUX/PADCONFIG registers, electricals, memory map |
| `am263p-controlcard-userguide-spruj86.pdf` | SPRUJ86 | TMDSCNCD263P ControlCARD EVM guide: LEDs, push buttons, GPIO mapping (Table 2-12), headers, switches, clocks |

## Wanted (download into this folder when Claude asks)

Name files `am263p-<short>-<docnum>.pdf`. Add a row above once present.

- [ ] AM263Px **SysConfig / MCU+ SDK** generated `ti_drivers_config.c` +
      `pinmux.h` for a GPIO example (only if needed to confirm pad offsets).

## How requests work

When Claude hits an unknown hardware fact it will state **exactly** which
document + section/table/register it needs. Download it here and tell Claude the
filename. Claude reads PDFs directly from this folder.

# AM263P4 Memory Map & Peripheral Base Addresses (verified)

Source of truth for the Zephyr port. Values pulled from the **TI MCU+ SDK**
(`mcupsdk-core`, branch `next`), cross-referenced with the AM263Px TRM (SPRUJ55).

- Base addresses: `source/drivers/hw_include/am263px/cslr_soc_baseaddress.h`
- R5F local view: `source/drivers/hw_include/am263px/cslr_soc_r5_baseaddress.h`
- Tested R5F memory layout (LP-AM263P, R5FSS0 core0):
  `examples/drivers/uart/uart_echo_interrupt_lld/am263px-lp/r5fss0-0_nortos/ti-arm-clang/linker.cmd`

---

## Memory regions — R5FSS0 Core 0 (the first-boot target)

This is TI's tested layout for a single core on the LaunchPad. Use it verbatim
for the Zephyr linker/DTS so we don't overlap other R5F cores.

| Region | Origin | Length | Use in Zephyr |
|---|---|---|---|
| R5F_VECS | `0x00000000` | `0x40` (64 B) | ARM vector table |
| R5F_TCMA | `0x00000040` | `0x7FC0` (~32 KB) | fast code (after vectors) |
| R5F_TCMB | `0x00080000` | `0x8000` (32 KB used) | fast data / stacks |
| **OCRAM** | **`0x70040000`** | **`0x40000` (256 KB)** | **`zephyr,sram`** — .text/.data/.bss/heap |
| FLASH (XIP) | `0x60100000` | `0x80000` (512 KB) | later (SBL/flash boot) |

Notes:
- L2OCRAM full window base is `0x70000000` (`CSL_L2OCRAM_U_BASE`). Each R5F core
  takes a non-overlapping 256 KB slice; core0 uses `0x70040000`.
- TCMA/TCMB *global* aliases (cross-core view): TCMA `0x78000000`,
  TCMB `0x78100000`. Local (core-private) view is `0x0` / `0x80000` as above.
- The CSL `cslr_soc_r5_baseaddress.h` lists TCMA=32 KB / TCMB=96 KB; the AM263P4
  datasheet advertises larger TCM (64 KB TCMA + 192 KB TCMB) in single-core mode.
  **First boot links into OCRAM**, so exact TCM size is not on the critical path —
  confirm against the AM263P4 datasheet before optimizing TCM placement.

---

## Peripheral base addresses (from cslr_soc_baseaddress.h)

| Peripheral | Macro | Base |
|---|---|---|
| **VIM** (interrupt mgr) | `CSL_VIM_U_BASE` | `0x50F00000` |
| **UART0** (console) | `CSL_UART0_U_BASE` | `0x52300000` |
| UART1..5 | `CSL_UART1..5_U_BASE` | `0x52301000` … `0x52305000` (0x1000 stride) |
| **RTI0** (sys timer) | `CSL_RTI0_U_BASE` | `0x52180000` |
| RTI1..7 | `CSL_RTI1..7_U_BASE` | `0x52181000` … `0x52187000` |
| **GPIO0** | `CSL_GPIO0_U_BASE` | `0x52000000` |
| GPIO1..3 | `CSL_GPIO1..3_U_BASE` | `0x52001000` … `0x52003000` |
| GPIO intr xbar | `CSL_GPIO_INTR_XBAR_U_BASE` | `0x52E02000` |
| **TOP_RCM** (clock) | `CSL_TOP_RCM_U_BASE` | `0x53200000` |
| **MSS_RCM** (clock) | `CSL_MSS_RCM_U_BASE` | `0x53208000` |
| EPWM0 (G0) | `CSL_CONTROLSS_G0_EPWM0_U_BASE` | `0x50000000` (0x1000 stride) |
| ADC0 cfg | `CSL_CONTROLSS_ADC0_U_BASE` | `0x502C0000` |
| ADC0 result | `CSL_CONTROLSS_ADC0_RESULT_U_BASE` | `0x50100000` |
| MCAN0 msg RAM | `CSL_MCAN0_MSG_RAM_U_BASE` | `0x52600000` |

---

## Implications for the port

- **Interrupt controller:** VIM at `0x50F00000` → reuse `drivers/interrupt_controller/intc_vim.c` (already in tree). DT node `&vim` with this reg.
- **Console:** UART0 at `0x52300000`, 16550-class → check `drivers/serial/` for an
  existing compatible driver; likely just DT wiring.
- **System tick:** RTI0 at `0x52180000`. Check `drivers/timer/` for an existing
  RTI/dmtimer driver.
- **Main RAM:** OCRAM `0x70040000` len 256 KB → `zephyr,sram` chosen node.
- **Clocks:** TOP_RCM / MSS_RCM — minimal PLL/clock init in soc.c; MCU+ SDK
  `soc_rcm.c` (am263px) is the reference for register sequences.

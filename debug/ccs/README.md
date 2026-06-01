# CCS Debug Folder for AM263P Zephyr (TMDSCNCD263P ControlCARD)

This folder exists so you can easily open a CCS debug session without hunting
for the ELF or re-creating the target configuration every time.

## Recommended Directory Layout (after using the helper)

```
debug/ccs/
├── README.md
├── refresh-elf.ps1               # Run after every build
├── tmdscncd263p_xds110_r5f0_0.ccxml   # Your saved target configuration
└── zephyr.elf                    # Latest build (copied by the script)
```

## One-time Setup (do this once)

### 1. Create / save the real .ccxml file (strongly recommended)

The `.ccxml` file is probe-specific (it stores your XDS110 serial number, etc.).
Do **not** try to hand-write it.

In CCS:
1. Go to **File → New → Target Configuration File**
2. Name it something like `tmdscncd263p_xds110_r5f0_0.ccxml` and save it into **this folder** (`debug/ccs/`).
3. In the Target Configuration editor:
   - **Connection**: Texas Instruments XDS110 USB Debug Probe
   - **Board or Device**: Search for `AM263P` or `AM263Px`
   - Under the device tree, **enable only Core_R5_0** (R5FSS0_0 / Cortex R5).
     - Disable all other cores (this is important for single-core bring-up).
   - Set the clock / JTAG frequency if needed (default usually works).
4. Click **Save** (or Test Connection).
5. Close the editor. The file should now live in this folder.

You can double-click the `.ccxml` later to open the debug session.

### 2. (Optional) Create a reusable Launch Configuration

- In CCS, after connecting once manually, you can create a `.launch` file:
  - Right-click in Debug view → **Edit Launch Configuration**
  - Or **Run → Debug Configurations...**
- Set it to:
  - Load the program from `zephyr.elf` in this folder (or use the absolute path to your build).
  - Connect only the R5FSS0_0 core.
- Save the `.launch` file into this folder for one-click "Launch".

## Daily Workflow (after every build)

You have two options — use whichever you prefer:

**Option A – PowerShell (recommended if you already use build.ps1):**
```powershell
.\debug\ccs\refresh-elf.ps1
```

**Option B – Command Prompt / cmd.exe (no PowerShell required):**
```cmd
debug\ccs\refresh-elf.cmd
```

Both scripts do exactly the same thing: copy the latest `build\zephyr\zephyr.elf` into this folder as `zephyr.elf`.

---

## Using the .launch File (Recommended)

Since you have `debug/ccs/` open as your workspace in CCS:

1. After running the refresh script, right-click on **`zephyr_r5f0_0.launch`** in the Project Explorer.
2. Choose **Debug As → zephyr_r5f0_0**.
3. It should:
   - Connect using the `.ccxml`
   - Load `zephyr.elf`
   - Reset and halt

This is currently the closest to a one-click debug experience in this folder.

> **Note:** The `.launch` file may need small adjustments the first time (especially the core name). If it doesn't work perfectly, you can still use the manual method below.

2. In CCS:
   - Double-click `tmdscncd263p_xds110_r5f0_0.ccxml` (or open it via the Target Configurations view).
   - Or use your saved `.launch` file.
   - **Connect** to the target (it should only attach to Core_R5_0).
   - **Load Program** → browse to `zephyr.elf` in this folder.
   - **Restart** (or Reset CPU).
   - **Resume** (F8).

3. Console output appears on the XDS110 virtual COM port at 115200 8N1.

## Current Known-Good Manual Steps (from bring-up)

- Always connect **only Core_R5_0**.
- The image is a **RAM load** (TCM + OCRAM @ 0x70040000). It is **not** flashed.

### Common Error: -215 "semaphore time-out"

This is extremely common with XDS110 on Windows.

**Quick fix (one-click):**
```cmd
debug\ccs\kill-xds110.cmd
```

Then:
1. Unplug the USB cable from the ControlCARD
2. Wait 5–10 seconds
3. Plug it back in
4. Restart CCS (strongly recommended)
5. Try connecting again

**Manual steps (if you prefer Task Manager):**
- Open Task Manager → Details tab
- End all instances of:
  - `DSLite.exe`
  - `xds2xx_fw_upgrade.exe`
  - `dbgsrv.exe`
- Unplug/replug the cable + restart CCS

The `kill-xds110.cmd` script does the above automatically.

## Files in This Folder

- `refresh-elf.ps1` – PowerShell version of the refresh script.
- `refresh-elf.cmd` – cmd.exe version of the refresh script.
- `kill-xds110.cmd` – cmd.exe helper to kill stale DSLite processes (fixes the common -215 error).
- `zephyr.elf` – Will appear after you run the refresh script (git-ignored).
- `tmdscncd263p_xds110_r5f0_0.ccxml` – Your CCS target configuration.
- `zephyr_r5f0_0.launch` – CCS debug launch configuration.
- `.theia/launch.json` – Old Theia config (marked deprecated).

## Notes

- Never commit `zephyr.elf` or any binary into git.
- The `.gitignore` at repo root should already ignore `debug/ccs/zephyr.elf` and `*.elf`.
- This setup keeps your real Zephyr build tree clean while giving CCS a simple place to point at.

For the overall port status, see `porting/ROADMAP.md` and `porting/IMPLEMENTATION_PLAN.md`.

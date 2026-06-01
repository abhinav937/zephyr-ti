# build.ps1
#
# One-command Zephyr build for the AM263P port.
#
# Defaults to the TMDSCNCD263P ControlCARD target, but board and sample are
# parameters now (there is more than one board), so:
#
#   .\build.ps1                                  # default board + sample
#   .\build.ps1 -Pristine                        # force a clean rebuild
#   .\build.ps1 -Board lp_am263p/am263p4/r5f0_0  # build the LaunchPad board
#   .\build.ps1 -Sample samples\synchronization
#   .\build.ps1 -t menuconfig                    # pass extra args to west build
#
# Incremental builds use `west build -p auto`: west itself does a pristine
# rebuild only when the build dir can't be reused (e.g. you switched boards),
# and a fast incremental otherwise. No manual object-file juggling needed.
#
# ZEPHYR_BASE is pinned because .west/ lives at the root of C:\ on this machine.

[CmdletBinding()]
param(
    [string]$Board  = "tmdscncd263p/am263p4/r5f0_0",
    [string]$Sample = "samples\hello_world",

    # Force a full clean rebuild (west -p always). Otherwise -p auto is used.
    [switch]$Pristine,

    [switch]$Help,

    # Extra arguments passed straight through to `west build` (e.g. -t menuconfig).
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$WestArgs
)

$ErrorActionPreference = "Stop"
$ZephyrDir = "C:\zephyr-ti"

if ($Help) {
    Write-Host @"
AM263P Zephyr build script

Usage:
  .\build.ps1                                  Build the default board + sample
  .\build.ps1 -Pristine                        Full clean rebuild (-p always)
  .\build.ps1 -Board <board> -Sample <path>    Override board / sample
  .\build.ps1 -t menuconfig                    Pass extra args to west build

Defaults:
  Board  : tmdscncd263p/am263p4/r5f0_0
  Sample : samples\hello_world

Sets ZEPHYR_BASE=$ZephyrDir (required because .west/ lives at the C:\ root).
"@
    exit 0
}

# ------------------------------------------------------------------
# Environment
# ------------------------------------------------------------------
Write-Host "==> Switching to $ZephyrDir ..." -ForegroundColor Cyan
Set-Location $ZephyrDir

$venvActivate = ".\.venv-zephyr\Scripts\Activate.ps1"
if (-not (Test-Path $venvActivate)) {
    Write-Host "ERROR: Python venv not found at $ZephyrDir\.venv-zephyr" -ForegroundColor Red
    Write-Host "Create it first: python -m venv .venv-zephyr" -ForegroundColor Yellow
    exit 1
}
Write-Host "==> Activating Python virtual environment..." -ForegroundColor Cyan
. $venvActivate

$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_BASE = $ZephyrDir
Write-Host "==> ZEPHYR_BASE pinned to $ZephyrDir (workaround for .west/ at C:\)" -ForegroundColor DarkGray

# ------------------------------------------------------------------
# Locate the Zephyr SDK toolchain
# ------------------------------------------------------------------
function Find-ZephyrSdk {
    if ($env:ZEPHYR_SDK_INSTALL_DIR -and (Test-Path $env:ZEPHYR_SDK_INSTALL_DIR)) {
        return $env:ZEPHYR_SDK_INSTALL_DIR
    }

    $candidates = @((Join-Path $HOME "zephyr-sdk-*"), "C:\zephyr-sdk-*")
    $found = Get-ChildItem -Path $candidates -Directory -ErrorAction SilentlyContinue |
             Sort-Object Name -Descending | Select-Object -First 1

    if (-not $found) {
        # Fallback: scan other user profiles.
        $found = Get-ChildItem -Path "C:\Users" -Directory -ErrorAction SilentlyContinue |
                 ForEach-Object { Get-ChildItem -Path (Join-Path $_.FullName "zephyr-sdk-*") -Directory -ErrorAction SilentlyContinue } |
                 Sort-Object Name -Descending | Select-Object -First 1
    }

    if ($found) { return $found.FullName }
    return $null
}

$sdkPath = Find-ZephyrSdk
if (-not $sdkPath) {
    Write-Host "ERROR: Could not find a Zephyr SDK." -ForegroundColor Red
    Write-Host "Set ZEPHYR_SDK_INSTALL_DIR or install the SDK in a standard location." -ForegroundColor Yellow
    exit 1
}

$toolchainBin = Join-Path $sdkPath "gnu\arm-zephyr-eabi\bin"
if (-not (Test-Path $toolchainBin)) {
    Write-Host "ERROR: Toolchain bin not found at $toolchainBin" -ForegroundColor Red
    exit 1
}
$env:ZEPHYR_SDK_INSTALL_DIR = $sdkPath
$env:Path = "$toolchainBin;$env:Path"
Write-Host "==> Using Zephyr SDK: $sdkPath" -ForegroundColor Green

# ------------------------------------------------------------------
# Ensure CMake is on PATH
# ------------------------------------------------------------------
function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    foreach ($path in @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\ProgramData\chocolatey\bin\cmake.exe")) {
        if (Test-Path $path) { return $path }
    }
    return $null
}

$cmakePath = Find-CMake
if (-not $cmakePath) {
    Write-Host "ERROR: CMake not found. Install it (https://cmake.org/download/ or 'choco install cmake')." -ForegroundColor Red
    exit 1
}
$cmakeDir = Split-Path $cmakePath
if ($env:Path -notlike "*$cmakeDir*") { $env:Path = "$cmakeDir;$env:Path" }
Write-Host "==> Using CMake: $cmakePath" -ForegroundColor Green

# ------------------------------------------------------------------
# Build
# ------------------------------------------------------------------
$pristineMode = if ($Pristine) { "always" } else { "auto" }
$buildArgs = @("-b", $Board, $Sample, "-p", $pristineMode)
if ($WestArgs) { $buildArgs += $WestArgs }

Write-Host "==> west build -b $Board $Sample -p $pristineMode $WestArgs" -ForegroundColor Cyan

west build @buildArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n==> Build FAILED." -ForegroundColor Red
    exit $LASTEXITCODE
}

# ------------------------------------------------------------------
# Success
# ------------------------------------------------------------------
$elfPath = Join-Path $ZephyrDir "build\zephyr\zephyr.elf"
Write-Host ""
Write-Host "==> Build OK: $elfPath" -ForegroundColor Green
Write-Host ""
Write-Host "Load over XDS110 (TI CCS):" -ForegroundColor Cyan
Write-Host "   1. Connect ONLY Core_R5_0" -ForegroundColor White
Write-Host "   2. Load Program -> $elfPath" -ForegroundColor White
Write-Host "   3. Restart -> Resume" -ForegroundColor White
Write-Host "   4. Console: XDS110 user-UART COM port @ 115200 8N1" -ForegroundColor White
Write-Host "   (-215 semaphore timeout = stale DSLite holding the probe; see debug/ccs/)" -ForegroundColor DarkGray

exit 0

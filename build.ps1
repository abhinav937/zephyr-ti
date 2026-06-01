# build.ps1
#
# Simple one-command build for the AM263P Zephyr port (TMDSCNCD263P ControlCARD).
#
# For now this script is intentionally locked to the current development target:
#   Board:  lp_am263p/am263p4/r5f0_0
#   Sample: samples/hello_world
#
# Just run:
#   .\build.ps1
#   .\build.ps1 -Pristine
#   .\build.ps1 -t menuconfig
#
# Double-click build.bat for the simplest experience (no typing needed).
#
# The script forces ZEPHYR_BASE because .west/ lives at the root of C:\ on this machine.

[CmdletBinding()]
param(
    [switch]$Pristine,
    [switch]$Help,

    # Any extra arguments (e.g. -t menuconfig) are passed through to west build
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$WestArgs
)

# Currently locked to the AM263P development target.
# This will be made configurable again later when we need multi-board support.
$Board  = "lp_am263p/am263p4/r5f0_0"
$Sample = "samples\hello_world"

$ErrorActionPreference = "Stop"

# Print build start time as early as possible
$scriptStartTime = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
Write-Host "==> Build script started at: $scriptStartTime" -ForegroundColor DarkGray

# Unique Build ID that appears in every build (normal or pristine)
$buildId = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
Write-Host "==> Build ID: $buildId" -ForegroundColor Yellow

# ------------------------------------------------------------------
# Help
# ------------------------------------------------------------------
if ($Help) {
    Write-Host @"
AM263P Zephyr Build Script (locked to current target)

Usage:
  .\build.ps1                 # Build hello_world for AM263P ControlCARD
  .\build.ps1 -Pristine       # Full clean rebuild
  .\build.ps1 -t menuconfig   # Pass extra arguments to west

This script is currently hardcoded to:
  Board:  lp_am263p/am263p4/r5f0_0
  Sample: samples/hello_world

It always sets ZEPHYR_BASE (required because .west/ lives at C:\ root on this machine).
"@
    exit 0
}

# ------------------------------------------------------------------
# Configuration
# ------------------------------------------------------------------
$ZephyrDir = "C:\zephyr-ti"

Write-Host "==> Switching to $ZephyrDir ..." -ForegroundColor Cyan
Set-Location $ZephyrDir

# ------------------------------------------------------------------
# Activate Python venv
# ------------------------------------------------------------------
$venvActivate = ".\.venv-zephyr\Scripts\Activate.ps1"
if (-not (Test-Path $venvActivate)) {
    Write-Host "ERROR: Python venv not found at $ZephyrDir\.venv-zephyr" -ForegroundColor Red
    Write-Host "Please create the venv first (python -m venv .venv-zephyr)." -ForegroundColor Yellow
    exit 1
}

Write-Host "==> Activating Python virtual environment..." -ForegroundColor Cyan
. $venvActivate

$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Critical workaround for this workspace layout
$env:ZEPHYR_BASE = $ZephyrDir
Write-Host "==> ZEPHYR_BASE pinned to $ZephyrDir (workaround for .west/ at C:\)" -ForegroundColor DarkGray

# ------------------------------------------------------------------
# Locate Zephyr SDK (smart detection)
# ------------------------------------------------------------------
function Find-ZephyrSdk {
    # 1. Explicit environment variable (highest priority)
    if ($env:ZEPHYR_SDK_INSTALL_DIR -and (Test-Path $env:ZEPHYR_SDK_INSTALL_DIR)) {
        return $env:ZEPHYR_SDK_INSTALL_DIR
    }

    # 2. Look for newest zephyr-sdk-* under common locations
    $candidates = @(
        (Join-Path $HOME "zephyr-sdk-*"),
        "C:\zephyr-sdk-*"
    )

    $found = Get-ChildItem -Path $candidates -Directory -ErrorAction SilentlyContinue |
             Sort-Object Name -Descending |
             Select-Object -First 1

    if (-not $found) {
        # Fallback: search under C:\Users for other user profiles (slower but reliable)
        $found = Get-ChildItem -Path "C:\Users" -Directory -ErrorAction SilentlyContinue |
                 ForEach-Object { Get-ChildItem -Path (Join-Path $_.FullName "zephyr-sdk-*") -Directory -ErrorAction SilentlyContinue } |
                 Sort-Object Name -Descending |
                 Select-Object -First 1
    }

    if ($found) {
        return $found.FullName
    }

    # 3. Last resort: old hardcoded path (will warn)
    $oldPath = "C:\Users\abhin\zephyr-sdk-1.0.1"
    if (Test-Path $oldPath) {
        Write-Host "WARNING: Using legacy hardcoded SDK path. Please set ZEPHYR_SDK_INSTALL_DIR instead." -ForegroundColor Yellow
        return $oldPath
    }

    return $null
}

$sdkPath = Find-ZephyrSdk

if (-not $sdkPath) {
    Write-Host "ERROR: Could not find Zephyr SDK." -ForegroundColor Red
    Write-Host "Set the environment variable ZEPHYR_SDK_INSTALL_DIR or install the SDK in a standard location." -ForegroundColor Yellow
    exit 1
}

$toolchainBin = Join-Path $sdkPath "gnu\arm-zephyr-eabi\bin"
if (-not (Test-Path $toolchainBin)) {
    Write-Host "ERROR: Toolchain bin directory not found at $toolchainBin" -ForegroundColor Red
    exit 1
}

# Set the official variable (best practice) and update PATH
$env:ZEPHYR_SDK_INSTALL_DIR = $sdkPath
$env:Path = "$toolchainBin;$env:Path"

Write-Host "==> Using Zephyr SDK: $sdkPath" -ForegroundColor Green

# ------------------------------------------------------------------
# Ensure CMake is available
# ------------------------------------------------------------------
function Find-CMake {
    # 1. Already in PATH?
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    # 2. Common installation locations
    $candidates = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\ProgramData\chocolatey\bin\cmake.exe"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

$cmakePath = Find-CMake

if ($cmakePath) {
    $cmakeDir = Split-Path $cmakePath
    if ($env:Path -notlike "*$cmakeDir*") {
        $env:Path = "$cmakeDir;$env:Path"
    }
    Write-Host "==> Using CMake: $cmakePath" -ForegroundColor Green
} else {
    Write-Host "ERROR: CMake not found." -ForegroundColor Red
    Write-Host ""
    Write-Host "Common fixes:" -ForegroundColor Yellow
    Write-Host "  - Install CMake from https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host "  - Or via Chocolatey: choco install cmake" -ForegroundColor Yellow
    Write-Host "  - Make sure CMake's bin folder is in your PATH" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# ------------------------------------------------------------------
# Build
# ------------------------------------------------------------------
$buildArgs = @("-b", $Board, $Sample)

if ($Pristine) {
    $buildArgs += "-p", "always"
    Write-Host "==> Building AM263P target (PRISTINE) ..." -ForegroundColor Cyan
} else {
    Write-Host "==> Building AM263P target (incremental) ..." -ForegroundColor Cyan

    # ------------------------------------------------------------------
    # Very aggressive rebuild for AM263P bring-up.
    # CMake's dependency tracking has been extremely unreliable on this machine
    # for soc/ and app files. To avoid constantly needing -Pristine, we now:
    #   - Always delete the object files for the files the user is actively editing
    #     on AM263P (even if git doesn't see them as changed).
    # This guarantees recompilation without a full pristine build.
    # ------------------------------------------------------------------
    if ($Board -like "*am263p*") {
        Write-Host "==> AM263P aggressive rebuild mode enabled" -ForegroundColor DarkGray

        $filesToNuke = @()

        # Core SoC file - this has been the #1 source of stale builds
        if (Test-Path "soc/ti/am263x/soc.c") {
            $filesToNuke += "soc/ti/am263x/soc.c"
        }

        # App debug code
        if ($Sample -like "*hello_world*") {
            if (Test-Path "samples/hello_world/src/main.c") {
                $filesToNuke += "samples/hello_world/src/main.c"
            }
        }

        foreach ($file in $filesToNuke) {
            if (Test-Path $file) {
                # Touch it
                (Get-Item $file).LastWriteTime = Get-Date

                # Delete the object file - this is the reliable hammer
                $relativePath = $file -replace '/', '\'
                $objFile = Join-Path "build\zephyr\CMakeFiles" "zephyr.dir\$relativePath.obj"

                if (-not (Test-Path $objFile)) {
                    $objFile = Join-Path "build\zephyr\CMakeFiles" "app.dir\$relativePath.obj"
                }

                if (Test-Path $objFile) {
                    Remove-Item $objFile -Force -ErrorAction SilentlyContinue
                    Write-Host "    -> Deleted object to force rebuild: $objFile" -ForegroundColor DarkGray
                }

                Write-Host "    -> Touched $file (AM263P force-rebuild rule)" -ForegroundColor DarkGray
            }
        }

        if ($filesToNuke.Count -gt 0) {
            Write-Host "==> Forced rebuild of key AM263P files (no -Pristine needed)" -ForegroundColor DarkGray
        }
    }

    # Still run git detection for any other files the user might have touched
    if (Test-Path ".git") {
        Write-Host "==> Detecting other changed source files via git..." -ForegroundColor DarkGray

        $changedFiles = @(
            git diff --name-only
            git diff --name-only --cached
            git ls-files --others --exclude-standard
        ) | Where-Object { $_ } | Sort-Object -Unique

        $sourceExtensions = @('.c', '.cpp', '.cxx', '.cc', '.S', '.s', '.h', '.hpp', '.hxx')

        $sourcesToTouch = $changedFiles | Where-Object {
            $ext = [System.IO.Path]::GetExtension($_).ToLower()
            $sourceExtensions -contains $ext
        }

        if ($sourcesToTouch.Count -gt 0) {
            Write-Host "==> Git detected $($sourcesToTouch.Count) other changed source file(s):" -ForegroundColor DarkGray
            foreach ($file in $sourcesToTouch) {
                if (Test-Path $file) {
                    (Get-Item $file).LastWriteTime = Get-Date
                    Write-Host "    -> Touched $file" -ForegroundColor DarkGray
                }
            }
        } else {
            Write-Host "==> No other source file changes detected by git" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "==> Not a git repository - skipping git change detection" -ForegroundColor Yellow
    }
}

Write-Host "==> Build started at: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  (ID: $buildId)" -ForegroundColor DarkGray

if ($WestArgs) {
    Write-Host "==> Passing extra arguments to west: $WestArgs" -ForegroundColor DarkGray
    $buildArgs += $WestArgs
}

# Use 'west' from the activated venv (more robust than hardcoding west.exe path)
west build @buildArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n==> Build FAILED." -ForegroundColor Red
    exit $LASTEXITCODE
}

# ------------------------------------------------------------------
# Success + Project-specific next steps
# ------------------------------------------------------------------
$elfPath = Join-Path $ZephyrDir "build\zephyr\zephyr.elf"

Write-Host ""
Write-Host "==> Build finished successfully at: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  (Build ID: $buildId)" -ForegroundColor Green
Write-Host "ELF: $elfPath" -ForegroundColor Yellow

Write-Host ""
Write-Host "==> Next steps for TMDSCNCD263P ControlCARD (AM263P):" -ForegroundColor Cyan
Write-Host "   1. In CCS, connect ONLY to Core_R5_0" -ForegroundColor White
Write-Host "   2. Load Program -> $elfPath" -ForegroundColor White
Write-Host "   3. Run -> Restart -> Resume" -ForegroundColor White
Write-Host "   4. Console: XDS110 user-UART COM port @ 115200 8N1" -ForegroundColor White
Write-Host ""
Write-Host "   (The -215 semaphore timeout usually means a stale DSLite is holding the probe.)" -ForegroundColor DarkGray
Write-Host ""

exit 0

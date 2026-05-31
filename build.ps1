# build.ps1
# Reliable one-click build for this workspace.
# Run from anywhere (double-click build.bat or run .\build.ps1).
# It will automatically switch into C:\zephyr-ti and build.

param(
    [string]$Board  = "lp_am263p/am263p4/r5f0_0",
    [string]$Sample = "samples\hello_world"
)

$ErrorActionPreference = "Stop"

$ZephyrDir = "C:\zephyr-ti"

Write-Host "==> Switching to $ZephyrDir ..." -ForegroundColor Cyan
Set-Location $ZephyrDir

if (-not (Test-Path ".\.venv-zephyr\Scripts\Activate.ps1")) {
    Write-Host "ERROR: Python venv not found at $ZephyrDir\.venv-zephyr" -ForegroundColor Red
    Write-Host "Please create the venv first." -ForegroundColor Yellow
    exit 1
}

Write-Host "==> Activating Python virtual environment..." -ForegroundColor Cyan
. .\.venv-zephyr\Scripts\Activate.ps1

$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Pin the workspace explicitly. Without this, west/CMake can resolve
# WEST_TOPDIR to C:\ (because .west lives at the drive root) and then fail
# with "Could not find a package configuration file provided by Zephyr".
$env:ZEPHYR_BASE = $ZephyrDir

Write-Host "==> Adding Zephyr SDK and build tools to PATH..." -ForegroundColor Cyan

$SdkToolchain = "C:\Users\abhin\zephyr-sdk-1.0.1\gnu\arm-zephyr-eabi\bin"
$Chocolatey   = "C:\ProgramData\chocolatey\bin"
$CMake        = "C:\Program Files\CMake\bin"

$env:Path = "$SdkToolchain;$Chocolatey;$CMake;" + $env:Path

Write-Host "==> Building board=$Board sample=$Sample ..." -ForegroundColor Cyan

.\.venv-zephyr\Scripts\west.exe build -b $Board $Sample -p always

Write-Host ""
Write-Host "==> Build finished." -ForegroundColor Green
Write-Host "ELF location: $ZephyrDir\build\zephyr\zephyr.elf" -ForegroundColor Yellow

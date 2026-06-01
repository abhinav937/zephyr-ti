<#
.SYNOPSIS
    Copies the latest Zephyr ELF into this CCS debug folder after a build.

.DESCRIPTION
    Run this script from the repo root (or from inside debug/ccs) after every
    successful .\build.ps1 or west build.

    It makes zephyr.elf available right next to your .ccxml and .launch files
    so CCS can easily find it for Load Program.

.EXAMPLE
    .\debug\ccs\refresh-elf.ps1
#>

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

# Resolve paths robustly whether the script is run from repo root or from inside debug/ccs
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot  = Resolve-Path (Join-Path $scriptDir '..\..')

$srcElf = Join-Path $repoRoot 'build\zephyr\zephyr.elf'
$dstDir = $scriptDir
$dstElf = Join-Path $dstDir 'zephyr.elf'

Write-Host "CCS Debug ELF refresh" -ForegroundColor Cyan
Write-Host "Repo root : $repoRoot"
Write-Host "Source    : $srcElf"

if (-not (Test-Path $srcElf)) {
    Write-Error "ERROR: Build output not found at:`n  $srcElf`n`nRun '.\build.ps1' first (or the equivalent west build)."
    exit 1
}

# Ensure destination folder exists
if (-not (Test-Path $dstDir)) {
    New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
}

Copy-Item -Path $srcElf -Destination $dstElf -Force

$sizeKB = [math]::Round((Get-Item $dstElf).Length / 1KB, 1)
$time   = (Get-Item $dstElf).LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss')

Write-Host ""
Write-Host "SUCCESS: Copied latest ELF for CCS debugging" -ForegroundColor Green
Write-Host "  Destination : $dstElf"
Write-Host "  Size        : $sizeKB KB"
Write-Host "  Timestamp   : $time"
Write-Host ""
Write-Host "Now in CCS you can Load Program from:" -ForegroundColor Yellow
Write-Host "  $dstElf" -ForegroundColor Yellow
Write-Host ""

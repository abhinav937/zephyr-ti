#!/bin/bash
#
# WSL Build Helper for Zephyr on AM263P (ControlCARD / LaunchPad)
#
# Usage:
#   1. Make it executable:
#        chmod +x wsl-build.sh
#
#   2. Run it:
#        ./wsl-build.sh
#
#   3. Optional: pass a different board or sample
#        ./wsl-build.sh -b lp_am263p/am263p4/r5f0_0 -s samples/synchronization
#
# IMPORTANT:
# - This script assumes you have the Linux Zephyr SDK installed.
#   Recommended location: ~/zephyr-sdk-1.0.1
# - You should create a Linux Python venv (not the Windows one).
# - For best performance, consider cloning the workspace inside WSL
#   instead of using /mnt/c/...

set -e

# ====================== CONFIGURATION ======================

# Change this if your workspace is somewhere else in WSL
ZEPHYR_WORKSPACE="/mnt/c/zephyr-ti"

# Where you installed the Linux Zephyr SDK (highly recommended)
ZEPHYR_SDK_DIR="$HOME/zephyr-sdk-1.0.1"

# Default board and sample
DEFAULT_BOARD="lp_am263p/am263p4/r5f0_0"
DEFAULT_SAMPLE="samples/hello_world"

# Python venv location (will be created inside the workspace if it doesn't exist)
VENV_DIR="$ZEPHYR_WORKSPACE/.venv-zephyr-wsl"

# ===========================================================

BOARD="$DEFAULT_BOARD"
SAMPLE="$DEFAULT_SAMPLE"

# Simple argument parsing
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--board)  BOARD="$2"; shift 2 ;;
        -s|--sample) SAMPLE="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo "==> Zephyr WSL Build Helper"
echo "    Workspace : $ZEPHYR_WORKSPACE"
echo "    Board     : $BOARD"
echo "    Sample    : $SAMPLE"
echo

if [ ! -d "$ZEPHYR_WORKSPACE" ]; then
    echo "ERROR: Workspace directory not found: $ZEPHYR_WORKSPACE"
    echo "Please update ZEPHYR_WORKSPACE in this script."
    exit 1
fi

cd "$ZEPHYR_WORKSPACE"

# --- Create / activate Linux venv ---
if [ ! -d "$VENV_DIR" ]; then
    echo "==> Creating Linux Python venv at $VENV_DIR ..."
    python3 -m venv "$VENV_DIR"
fi

echo "==> Activating venv..."
source "$VENV_DIR/bin/activate"

# --- Set Zephyr environment ---
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_BASE="$ZEPHYR_WORKSPACE"

# --- Add Zephyr SDK to PATH (Linux version) ---
if [ -d "$ZEPHYR_SDK_DIR" ]; then
    export PATH="$ZEPHYR_SDK_DIR/arm-zephyr-eabi/bin:$PATH"
else
    echo "WARNING: Zephyr SDK not found at $ZEPHYR_SDK_DIR"
    echo "Please install the Linux version of the SDK and update the path in this script."
fi

# --- Run west build ---
echo "==> Running west build ..."
west build -b "$BOARD" "$SAMPLE" -p auto

echo
echo "==> Build complete!"
echo "    ELF: $ZEPHYR_WORKSPACE/build/zephyr/zephyr.elf"

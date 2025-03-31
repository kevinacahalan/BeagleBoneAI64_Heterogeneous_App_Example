#!/bin/bash
# Set SCRIPT_DIR to the absolute path of the script's directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Check the device model
DEVICE_MODEL=$(cat /proc/device-tree/model | sed "s/ /_/g" | tr -d '\000')
if [ "$DEVICE_MODEL" != "BeagleBoard.org_BeagleBone_AI-64" ]; then
    echo "Error: This script should only be run on a BeagleBoneAI64."
    exit 1
fi

# Define source and destination directories
DTB_SRC="$SCRIPT_DIR/BeagleBoard-DeviceTrees"

# Remove the target directory if it exists
rm -rf "$DTB_SRC"
# Clone the repository into the specified directory
git clone --branch v6.12.x-Beagle --single-branch https://github.com/beagleboard/BeagleBoard-DeviceTrees.git "$DTB_SRC"

# Copy the overlay file
cp "$SCRIPT_DIR/our-custom-bbai64-overlay.dtso" "$DTB_SRC/src/arm64/overlays/"

# Build the device trees
make -C "$DTB_SRC" -f Makefile clean
make -C "$DTB_SRC" -f Makefile

# Install device tree stuff
sudo make -C "$DTB_SRC" -f Makefile install_arm64

# Copy over extlinux.conf
sudo cp -rf "$SCRIPT_DIR/kernel6-12-extlinux.conf" "/boot/firmware/extlinux/extlinux.conf"

echo "Device trees setup"
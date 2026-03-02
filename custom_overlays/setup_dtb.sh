#!/bin/bash
# Set SCRIPT_DIR to the absolute path of the script's directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

show_help() {
    echo "Usage: $0 [6.12|6.19]"
    echo ""
    echo "If no argument is provided, you will be prompted to choose."
    echo "Valid options:"
    echo "  6.12  -> branch v6.12.x-Beagle, kernel6-12-extlinux.conf"
    echo "  6.19  -> branch v6.19.x, kernel6-19-extlinux.conf"
}

KERNEL_VERSION="$1"

if [ -z "$KERNEL_VERSION" ]; then
    echo "Select kernel version:"
    echo "  1) 6.12"
    echo "  2) 6.19"
    read -r -p "Enter selection [1/2 or 6.12/6.19]: " KERNEL_VERSION

    case "$KERNEL_VERSION" in
        1)
            KERNEL_VERSION="6.12"
            ;;
        2)
            KERNEL_VERSION="6.19"
            ;;
        "")
            show_help
            exit 1
            ;;
    esac
fi

case "$KERNEL_VERSION" in
    6.12)
        DTB_BRANCH="v6.12.x-Beagle"
        EXTLINUX_CONF="$SCRIPT_DIR/kernel6-12-extlinux.conf"
        ;;
    6.19)
        DTB_BRANCH="v6.19.x"
        EXTLINUX_CONF="$SCRIPT_DIR/kernel6-19-extlinux.conf"
        ;;
    *)
        show_help
        exit 1
        ;;
esac

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
git clone --branch "$DTB_BRANCH" --single-branch https://github.com/beagleboard/BeagleBoard-DeviceTrees.git "$DTB_SRC"

# Copy the overlay file
cp "$SCRIPT_DIR/our-custom-bbai64-overlay.dtso" "$DTB_SRC/src/arm64/overlays/"

# Build the device trees
make -C "$DTB_SRC" -f Makefile clean
make -C "$DTB_SRC" -f Makefile

# Install device tree stuff
sudo make -C "$DTB_SRC" -f Makefile install_arm64

# Copy over extlinux.conf
sudo cp -rf "$EXTLINUX_CONF" "/boot/firmware/extlinux/extlinux.conf"

echo "Device trees setup"
echo ""
echo "WARNING! THIS SCRIPT MAY BREAK THINGS...IF YOU ARE USING AN IMAGE FROM BEFORE JULY 2025...RIP"
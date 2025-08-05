#!/bin/bash
# This script builds for the BeagleBone and copies the files to the BeagleBone.
# It requires the IP address of the BeagleBone to be provided as an argument.

# Check if the script is being run with sudo (as root)
if [ "$EUID" -eq 0 ]; then
    echo "Error: This script should not be run with sudo. Please run it as a regular user."
    exit 1
fi

SCRIPT_DIR=$(dirname "$0")

BUILD_SCRIPT="$SCRIPT_DIR/build_script.sh --beaglebone"
RSYNC_SOURCE_DIR="$SCRIPT_DIR/../"
RSYNC_DEST_DIR="~/BeagleBoneAI64_Heterogeneous_App_Example/"
BEAGLEBONE_IP=""  # No default IP address

# Function to print the help message
print_help() {
    echo "Usage: $0 --ip <BeagleBone IP> [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --ip              Specify the IP address of the BeagleBone. This option is required."
    echo "  help              Display this help message."
    echo ""
    exit 0
}

# Parse command-line options
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --ip)
            BEAGLEBONE_IP="$2"
            shift 2
            ;;
        help)
            print_help
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            ;;
    esac
done

# Check if IP address is provided
if [ -z "$BEAGLEBONE_IP" ]; then
    echo "Error: No IP address provided. Use --ip <BeagleBone IP>."
    exit 1
fi

# Run the build script
echo "Running build script for BeagleBone..."
$BUILD_SCRIPT
if [ $? -ne 0 ]; then
    echo "Build failed. Exiting."
    exit 1
fi

# Rsync the files over to the BeagleBone
echo "Build successful. Copying files to BeagleBone..."
rsync -av --exclude=".*" "$RSYNC_SOURCE_DIR" "debian@$BEAGLEBONE_IP:$RSYNC_DEST_DIR"
if [ $? -ne 0 ]; then
    echo "File copy failed. Exiting."
    exit 1
fi

echo "All tasks completed successfully."

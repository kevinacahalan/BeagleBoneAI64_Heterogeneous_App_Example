#!/bin/bash
# This script builds for the BeagleBone and copies the files to the BeagleBone.
# It requires an SSH destination: host alias from ~/.ssh/config, or user@host.

# Check if the script is being run with sudo (as root)
if [ "$EUID" -eq 0 ]; then
    echo "Error: This script should not be run with sudo. Please run it as a regular user."
    exit 1
fi

SCRIPT_DIR=$(dirname "$0")

BUILD_SCRIPT="$SCRIPT_DIR/build.sh --all"
RSYNC_SOURCE_DIR="$SCRIPT_DIR/../"
RSYNC_DEST_DIR="~/BeagleBoneAI64_Heterogeneous_App_Example/"
SSH_DEST=""  # SSH host alias or user@host

# Function to print the help message
print_help() {
    echo "Usage: $0 --ssh <destination> [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --ssh             SSH destination for rsync (required)."
    echo "                    Examples: bbai64  or  kevinc@192.168.7.2"
    echo "                    Host aliases use settings from ~/.ssh/config."
    echo "  help              Display this help message."
    echo ""
    exit 0
}

# Parse command-line options
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --ssh)
            SSH_DEST="$2"
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

# Check if SSH destination is provided
if [ -z "$SSH_DEST" ]; then
    echo "Error: No SSH destination provided. Use --ssh <host alias or user@host>."
    exit 1
fi

RSYNC_REMOTE="${SSH_DEST}:${RSYNC_DEST_DIR}"

# Run the build script
echo "Running build script for BeagleBone..."
$BUILD_SCRIPT
if [ $? -ne 0 ]; then
    echo "Build failed. Exiting."
    exit 1
fi

# Rsync the files over to the BeagleBone
echo "Build successful. Copying files to BeagleBone (${RSYNC_REMOTE})..."
rsync -av --exclude=".*" "$RSYNC_SOURCE_DIR" "$RSYNC_REMOTE"
if [ $? -ne 0 ]; then
    echo "File copy failed. Exiting."
    exit 1
fi

echo "All tasks completed successfully."

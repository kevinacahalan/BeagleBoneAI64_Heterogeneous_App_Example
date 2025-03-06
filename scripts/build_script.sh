#!/bin/bash
# This script builds the Linux and R5 firmware code.
# It supports options for building on the local development machine, the BeagleBone, or both.
# It also supports an optional --release flag for release builds, and a --clean flag for cleaning only.

SCRIPT_DIR=$(dirname "$0")
BUILD_TYPE=""
TARGET=""
CLEAN_ONLY=""

# Function to print the help message
print_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --local           Build for the local development machine."
    echo "  --beaglebone      Cross compile and build Linux code for the BeagleBone."
    echo "  --both            Build for both the local machine and the BeagleBone."
    echo "  --release         Perform a release build."
    echo "  --clean           Clean all build files without building."
    echo "  help              Display this help message."
    echo ""
    echo "Example:"
    echo "  $0 --local --release"
    echo "  $0 --beaglebone"
    echo "  $0 --both"
    echo "  $0 --clean"
    exit 0
}

# Parse command-line options
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --release)
            BUILD_TYPE="release"
            shift
            ;;
        --local)
            TARGET="local"
            shift
            ;;
        --beaglebone)
            TARGET="beaglebone"
            shift
            ;;
        --both)
            TARGET="both"
            shift
            ;;
        --clean)
            CLEAN_ONLY="true"
            shift
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

# If the clean option is set, clean everything and exit
if [ "$CLEAN_ONLY" = "true" ]; then
    echo "Cleaning all build files..."
    make -C "$SCRIPT_DIR/../LINUX_SIDE" clean || { echo "Clean failed for Linux code"; exit 1; }
    make -C "$SCRIPT_DIR/../R5_SIDE" clean || { echo "Clean failed for R5 firmware"; exit 1; }
    echo "Clean completed successfully."
    exit 0
fi

if [ -z "$TARGET" ]; then
    echo "Error: No target specified. Use --local, --beaglebone, --both, or --clean."
    print_help
    exit 1
fi

echo "Cleaning Linux build"
make -C "$SCRIPT_DIR/../LINUX_SIDE" clean || { echo "Clean failed for Linux code on BeagleBone"; exit 1; }

echo "Starting the build process..."

# Build Linux code for the local development machine,
if [ "$TARGET" = "local" ] || [ "$TARGET" = "both" ]; then
    echo "Building Linux code for local development machine..."
    if [ "$BUILD_TYPE" = "release" ]; then
        make -C "$SCRIPT_DIR/../LINUX_SIDE" release || { echo "Release build failed for Linux code on local machine"; exit 1; }
    else
        make -C "$SCRIPT_DIR/../LINUX_SIDE" || { echo "Build failed for Linux code on local machine"; exit 1; }
    fi
fi

# Build both R5 and linux code for the BeagleBone
if [ "$TARGET" = "beaglebone" ] || [ "$TARGET" = "both" ]; then

    echo "Building Linux code for BeagleBone..."
    if [ "$BUILD_TYPE" = "release" ]; then
        make -C "$SCRIPT_DIR/../LINUX_SIDE" CROSS_COMPILE=true release || { echo "Release build failed for Linux code on BeagleBone"; exit 1; }
    else
        make -C "$SCRIPT_DIR/../LINUX_SIDE" CROSS_COMPILE=true || { echo "Build failed for Linux code on BeagleBone"; exit 1; }
    fi

    # Build R5 code, only happens if cross-compiling
    echo "Cleaning R5 firmware build..."
    make -C "$SCRIPT_DIR/../R5_SIDE" clean || { echo "Clean failed for R5 firmware"; exit 1; }

    echo "Building R5 firmware code..."
    if [ "$BUILD_TYPE" = "release" ]; then
        make -C "$SCRIPT_DIR/../R5_SIDE" release || { echo "Release build failed for R5 firmware"; exit 1; }
    else
        make -C "$SCRIPT_DIR/../R5_SIDE" || { echo "Build failed for R5 firmware"; exit 1; }
    fi
fi

echo "Build process completed successfully."

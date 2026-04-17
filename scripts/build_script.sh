#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_MODE="debug"
TARGET="both"
CLEAN_ONLY=false

print_header() {
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}============================================================${NC}"
}

print_info() {
    echo -e "${CYAN}[info]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[ok]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[warn]${NC} $1"
}

print_error() {
    echo -e "${RED}[error]${NC} $1" >&2
}

print_help() {
    cat << EOF
Example build script

Usage:
    ./build_script.sh [TARGET] [OPTIONS]

Targets:
    --linux          Build only the Linux side for the BeagleBone target
    --r5             Build only the R5 firmware
    --both           Build both targets (default)

Options:
    --debug          Debug build for Linux outputs (default)
    --release        Release build for Linux outputs
    --clean          Clean Linux and R5 build artifacts and exit
    --help           Show this help text

Notes:
    - BUILD_MODE affects compiler flags for both Linux and R5 sources.
    - Debug means -Og -g3 for Linux and -g3 -Og for R5.
    - Release means -O3 -DNDEBUG for both Linux and R5 sources.
    - TI PDK library selection for R5 is unchanged by BUILD_MODE.

Examples:
    ./build_script.sh --both
    ./build_script.sh --linux --release
    ./build_script.sh --r5
    ./build_script.sh --both --release
    ./build_script.sh --clean
EOF
}

validate_not_root() {
    if [ "$EUID" -eq 0 ]; then
        print_error "Run this script as a regular user, not with sudo."
        return 1
    fi
    return 0
}

check_dependencies() {
    local missing=0

    if ! command -v make >/dev/null 2>&1; then
        print_error "make is not installed"
        missing=1
    fi

    if [ "$TARGET" = "linux" ] || [ "$TARGET" = "both" ]; then
        if [ "$(uname -m)" = "x86_64" ]; then
            if ! command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
                print_error "aarch64-linux-gnu-gcc is not installed"
                missing=1
            fi
        elif ! command -v gcc >/dev/null 2>&1; then
            print_error "gcc is not installed"
            missing=1
        fi
    fi

    if [ "$TARGET" = "r5" ] || [ "$TARGET" = "both" ]; then
        if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
            print_error "arm-none-eabi-gcc is not installed"
            missing=1
        fi
    fi

    return $missing
}

clean_all() {
    print_header "Cleaning build artifacts"
    make -C "$PROJECT_ROOT/LINUX_SIDE" clean
    make -C "$PROJECT_ROOT/R5_SIDE" clean
    print_success "Clean completed"
}

build_linux() {
    local make_args=(BUILD_MODE="$BUILD_MODE")

    if [ "$(uname -m)" = "x86_64" ]; then
        make_args+=(CROSS_COMPILE=true)
    fi

    print_header "Building Linux side for BeagleBone target"
    print_info "BUILD_MODE=$BUILD_MODE"
    make -C "$PROJECT_ROOT/LINUX_SIDE" "${make_args[@]}"
    print_success "Built $PROJECT_ROOT/build/linux/LINUX_SIDE_aarch64"
}

build_r5() {
    print_header "Building R5 firmware"
    print_info "BUILD_MODE=$BUILD_MODE"
    make -C "$PROJECT_ROOT/R5_SIDE" BUILD_MODE="$BUILD_MODE"
    print_success "Built $PROJECT_ROOT/build/R5_0/r5f_r5f0_0.elf"
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_MODE="debug"
            shift
            ;;
        --release)
            BUILD_MODE="release"
            shift
            ;;
        --linux)
            TARGET="linux"
            shift
            ;;
        --r5)
            TARGET="r5"
            shift
            ;;
        --both)
            TARGET="both"
            shift
            ;;
        --clean)
            CLEAN_ONLY=true
            shift
            ;;
        --help|-h|help)
            print_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

validate_not_root || exit 1

if [ "$CLEAN_ONLY" = true ]; then
    clean_all
    exit 0
fi

if ! check_dependencies; then
    exit 1
fi

case "$TARGET" in
    linux)
        build_linux
        ;;
    r5)
        build_r5
        ;;
    both)
        build_linux
        build_r5
        ;;
esac

print_success "Build process completed successfully"

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

TARGET=""
BUILD_MODE="debug"
CLEAN_ONLY=false
SHOW_HELP=false
ACTION_REQUESTED=false

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
Example build script (container-backed)

Usage:
    ./build_script.sh [TARGET] [OPTIONS]

Run with no arguments to show this help.

Targets:
    --linux          Build only the Linux side for the BeagleBone target
    --r5             Build only the R5 firmware
    --both           Build both targets

Options:
    --debug          Debug build (default)
    --release        Release build
    --clean          Clean Linux and R5 build artifacts and exit
    --help           Show this help text

Notes:
    Builds run inside Podman/Docker via docker_cross_build.sh.
    First-time R5 setup: ./scripts/docker_cross_build.sh --setup

Examples:
    ./build_script.sh --both
    ./build_script.sh --linux --release
    ./build_script.sh --r5
    ./build_script.sh --clean --both
EOF
}

print_intro() {
    print_header "BeagleBone AI64 build script"
    print_info "Project: ${PROJECT_ROOT}"
    print_info "Builds run inside Podman/Docker (see scripts/docker_cross_build.sh)."
    print_info "First-time R5 setup: ./scripts/docker_cross_build.sh --setup"
    echo
}

validate_not_root() {
    if [ "$EUID" -eq 0 ]; then
        print_error "Run this script as a regular user, not with sudo."
        return 1
    fi
    return 0
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_MODE="debug"
            ACTION_REQUESTED=true
            shift
            ;;
        --release)
            BUILD_MODE="release"
            ACTION_REQUESTED=true
            shift
            ;;
        --linux)
            TARGET="linux"
            ACTION_REQUESTED=true
            shift
            ;;
        --r5)
            TARGET="r5"
            ACTION_REQUESTED=true
            shift
            ;;
        --both)
            TARGET="both"
            ACTION_REQUESTED=true
            shift
            ;;
        --clean)
            CLEAN_ONLY=true
            ACTION_REQUESTED=true
            shift
            ;;
        --help|-h|help)
            SHOW_HELP=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            echo
            print_help
            exit 1
            ;;
    esac
done

if [[ "${SHOW_HELP}" == true ]] || [[ "${ACTION_REQUESTED}" != true ]]; then
    print_intro
    print_help
    exit 0
fi

validate_not_root || exit 1

if [[ "${CLEAN_ONLY}" != true && -z "${TARGET}" ]]; then
    print_error "A target is required: --linux, --r5, or --both"
    echo
    print_help
    exit 1
fi

if [[ "${CLEAN_ONLY}" == true && -z "${TARGET}" ]]; then
    TARGET="both"
fi

ARGS=()
if [[ "${CLEAN_ONLY}" == true ]]; then
  ARGS+=("--clean" "--${TARGET}")
  print_header "Cleaning build artifacts"
  print_info "Target=${TARGET}"
else
  ARGS+=("--${TARGET}")
  if [[ "${BUILD_MODE}" == "release" ]]; then
    ARGS+=("--release")
  else
    ARGS+=("--debug")
  fi
  print_header "Building (${TARGET}, ${BUILD_MODE})"
  print_info "Delegating to docker_cross_build.sh"
fi

"${SCRIPT_DIR}/docker_cross_build.sh" "${ARGS[@]}"
print_success "Build process completed successfully"

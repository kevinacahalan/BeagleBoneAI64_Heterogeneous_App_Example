#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=sdk_config.sh
source "${SCRIPT_DIR}/sdk_config.sh"

TI_SDK_DIR="${TI_SDK_DIR_DEFAULT}"
# Default: build both profiles so debug and release R5 firmware can link.
BUILD_PROFILES=("debug" "release")
SHOW_HELP="false"
PROFILE_OVERRIDE=""

print_help() {
    cat <<EOF
Usage: ./scripts/build_pdk_libs.sh [options]

Build PDK libraries required by the R5 firmware (runs inside the TI container).
By default builds both debug and release profiles.

Options:
    --ti-sdk-dir <path>   SDK install directory (default: \$HOME/ti)
    --debug               Build only debug PDK libs
    --release             Build only release PDK libs
    -h, --help            Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --ti-sdk-dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: --ti-sdk-dir requires a value" >&2
                exit 1
            fi
            TI_SDK_DIR="$2"
            shift 2
            ;;
        --debug)
            PROFILE_OVERRIDE="debug"
            shift
            ;;
        --release)
            PROFILE_OVERRIDE="release"
            shift
            ;;
        -h|--help)
            SHOW_HELP="true"
            shift
            ;;
        *)
            echo "Error: Unknown option: $1" >&2
            print_help
            exit 1
            ;;
    esac
done

if [[ "${SHOW_HELP}" == "true" ]]; then
    print_help
    exit 0
fi

if [[ -n "${PROFILE_OVERRIDE}" ]]; then
    BUILD_PROFILES=("${PROFILE_OVERRIDE}")
fi

SDK_ROOT="${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
if [[ ! -d "${SDK_ROOT}" ]]; then
    echo "Error: SDK not found at ${SDK_ROOT}" >&2
    echo "Run ./scripts/fetch_ti_sdk.sh first (or ./scripts/docker_cross_build.sh --fetch-sdk)." >&2
    exit 1
fi

PDK_PATH="$(sdk_resolve_pdk_path "${SDK_ROOT}")"
echo "Building PDK libraries (profiles: ${BUILD_PROFILES[*]}) in ${PDK_PATH}/packages"

export TOOLCHAIN_PATH_GCC=/usr
export TOOLCHAIN_PATH_GCC_ARCH64=/usr
export GCC_ARCH64_BIN_PREFIX=arm-none-eabi

for profile in "${BUILD_PROFILES[@]}"; do
    echo "Building PDK all_libs BUILD_PROFILE=${profile} ..."
    make -C "${PDK_PATH}/packages" -s all_libs BUILD_PROFILE="${profile}"
done

echo "PDK library build completed."

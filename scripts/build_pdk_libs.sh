#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=sdk_config.sh
source "${SCRIPT_DIR}/sdk_config.sh"

TI_SDK_DIR="${TI_SDK_DIR_DEFAULT}"
BUILD_PROFILE="debug"
SHOW_HELP="false"

print_help() {
    cat <<EOF
Usage: ./scripts/build_pdk_libs.sh [options]

Build PDK libraries required by the R5 firmware (runs inside the TI container).

Options:
    --ti-sdk-dir <path>   SDK install directory (default: \$HOME/ti)
    --release             Build release PDK libs instead of debug
    -h, --help            Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --ti-sdk-dir)
            TI_SDK_DIR="$2"
            shift 2
            ;;
        --release)
            BUILD_PROFILE="release"
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

SDK_ROOT="${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
if [[ ! -d "${SDK_ROOT}" ]]; then
    echo "Error: SDK not found at ${SDK_ROOT}" >&2
    echo "Run ./scripts/fetch_ti_sdk.sh first (or ./scripts/docker_cross_build.sh --fetch-sdk)." >&2
    exit 1
fi

PDK_PATH="$(sdk_resolve_pdk_path "${SDK_ROOT}")"
echo "Building PDK libraries (BUILD_PROFILE=${BUILD_PROFILE}) in ${PDK_PATH}/packages"

export TOOLCHAIN_PATH_GCC=/usr
export TOOLCHAIN_PATH_GCC_ARCH64=/usr
export GCC_ARCH64_BIN_PREFIX=arm-none-eabi

make -C "${PDK_PATH}/packages" -s all_libs BUILD_PROFILE="${BUILD_PROFILE}"

echo "PDK library build completed."

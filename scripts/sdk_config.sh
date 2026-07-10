#!/usr/bin/env bash
# Shared TI Processor SDK RTOS (J721E) version and path constants.
# Source this file from other build scripts; do not execute directly.

TI_SDK_VERSION="11_02_01_03"
TI_SDK_TARBALL="ti-processor-sdk-rtos-j721e-evm-${TI_SDK_VERSION}.tar.gz"
TI_SDK_URL="https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-bA0wfI4X2g/11.02.01.03/${TI_SDK_TARBALL}"
TI_SDK_ROOT_NAME="ti-processor-sdk-rtos-j721e-evm-${TI_SDK_VERSION}"

# Default host install directory (mounted at /home/builder/ti in the TI container).
TI_SDK_DIR_DEFAULT="${HOME}/ti"

# Container image names
DEBIAN_IMAGE_NAME="localhost/debian13-bbai64-build:latest"
TI_IMAGE_NAME="localhost/ti-bbai64-build:latest"

# Resolve PDK path under an extracted SDK root (prints path or empty).
sdk_resolve_pdk_path() {
    local sdk_root="$1"
    local pdk_path

    if [[ -z "${sdk_root}" || ! -d "${sdk_root}" ]]; then
        return 1
    fi

    # shellcheck disable=SC2012
    pdk_path="$(ls -d "${sdk_root}"/pdk_jacinto_* 2>/dev/null | head -n 1 || true)"
    if [[ -n "${pdk_path}" && -d "${pdk_path}" ]]; then
        printf '%s\n' "${pdk_path}"
        return 0
    fi

    return 1
}

#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=sdk_config.sh
source "${SCRIPT_DIR}/sdk_config.sh"

TI_SDK_DIR="${TI_SDK_DIR_DEFAULT}"
SHOW_HELP="false"

print_help() {
    cat <<EOF
Usage: ./scripts/fetch_ti_sdk.sh [options]

Download and extract Processor SDK RTOS ${TI_SDK_VERSION} for J721E if not already present.

Options:
    --ti-sdk-dir <path>   Install directory (default: \$HOME/ti)
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

mkdir -p "${TI_SDK_DIR}"

if [[ ! -w "${TI_SDK_DIR}" ]]; then
    echo "Error: TI SDK directory is not writable: ${TI_SDK_DIR}" >&2
    echo "Fix with: sudo chown -R \"\$(id -u):\$(id -g)\" \"${TI_SDK_DIR}\"" >&2
    exit 1
fi

SDK_ROOT="${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
TARBALL_PATH="${TI_SDK_DIR}/${TI_SDK_TARBALL}"

sdk_is_complete() {
    local root="$1"
    local pdk

    if [[ ! -d "${root}" ]]; then
        return 1
    fi
    pdk="$(sdk_resolve_pdk_path "${root}" 2>/dev/null || true)"
    [[ -n "${pdk}" && -d "${pdk}/packages" ]]
}

if sdk_is_complete "${SDK_ROOT}"; then
    echo "SDK already extracted at ${SDK_ROOT}"
else
    if [[ -d "${SDK_ROOT}" ]]; then
        echo "Incomplete SDK tree at ${SDK_ROOT}; removing and re-extracting ..."
        rm -rf "${SDK_ROOT}"
    fi

    if [[ ! -f "${TARBALL_PATH}" ]]; then
        echo "Downloading ${TI_SDK_TARBALL} ..."
        wget -c -O "${TARBALL_PATH}" "${TI_SDK_URL}"
    else
        echo "Using existing tarball ${TARBALL_PATH}"
    fi

    if [[ ! -s "${TARBALL_PATH}" ]]; then
        echo "Error: tarball is missing or empty: ${TARBALL_PATH}" >&2
        exit 1
    fi

    echo "Verifying tarball listing ..."
    if ! tar -tzf "${TARBALL_PATH}" >/dev/null; then
        echo "Error: tarball appears corrupt: ${TARBALL_PATH}" >&2
        echo "Delete it and re-run --fetch-sdk." >&2
        exit 1
    fi

    echo "Extracting ${TI_SDK_TARBALL} into ${TI_SDK_DIR} ..."
    if ! tar -xzf "${TARBALL_PATH}" -C "${TI_SDK_DIR}"; then
        echo "Error: extract failed; removing partial ${SDK_ROOT}" >&2
        rm -rf "${SDK_ROOT}"
        exit 1
    fi
fi

PDK_PATH="$(sdk_resolve_pdk_path "${SDK_ROOT}" || true)"
if [[ -z "${PDK_PATH}" ]]; then
    echo "Error: Could not find a single pdk_jacinto_* under ${SDK_ROOT}" >&2
    exit 1
fi

echo "SDK root: ${SDK_ROOT}"
echo "PDK path: ${PDK_PATH}"

#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
# shellcheck source=sdk_config.sh
source "${SCRIPT_DIR}/sdk_config.sh"

DEBIAN_IMAGE="${DEBIAN_IMAGE_NAME}"
TI_IMAGE="${TI_IMAGE_NAME}"
DEBIAN_DOCKERFILE="${REPO_ROOT}/docker/Dockerfile.debian13"
TI_DOCKERFILE="${REPO_ROOT}/docker/Dockerfile.ti"

TARGET=""
EXPLICIT_TARGET=""
BUILD_MODE="debug"
TI_SDK_DIR="${TI_SDK_DIR_DEFAULT}"
SKIP_IMAGE_BUILD="false"
FETCH_SDK="false"
BUILD_PDK="false"
SETUP_ONLY="false"
CONTAINER_ENGINE=""
MOUNT_SUFFIX=""
USER_FLAGS=()
CLEAN_ONLY="false"
ACTION_REQUESTED="false"

print_help() {
    cat <<EOF
Usage: ./scripts/docker_cross_build.sh [options]

Build inside containers (Podman or Docker):
  - Debian 13 image: Linux aarch64 cross-build (gpiod v2)
  - TI ubuntu-distro image: R5 firmware, SDK fetch, PDK libs

Run with no arguments to show this help.

Options:
    --linux                Build only the Linux side for BeagleBone (aarch64).
    --r5                   Build only the R5 firmware.
    --both                 Build both targets.
    --debug                Debug build (default): app flags + PDK debug libs.
    --release              Release build: app flags + PDK release libs.
    --clean                Clean Linux and R5 build artifacts and exit.
    --fetch-sdk            Download/extract TI SDK ${TI_SDK_VERSION} (TI container).
    --build-pdk            Build PDK debug and release libraries (TI container).
    --setup                Shorthand for --fetch-sdk --build-pdk (TI container only).
    --ti-sdk-dir <path>    Host path for TI SDK. Default: \$HOME/ti
    --skip-image-build     Reuse existing images; skip docker/podman build.
    -h, --help             Show this help.

Notes:
    - BUILD_MODE selects compiler flags and matching PDK library profile for R5.
    - --setup builds both PDK debug and release profiles.
    - First-time setup: ./scripts/docker_cross_build.sh --setup
    - SDK ${TI_SDK_VERSION} is mounted at /home/builder/ti in the TI container.

Examples:
    ./scripts/docker_cross_build.sh --setup
    ./scripts/docker_cross_build.sh --both
    ./scripts/docker_cross_build.sh --linux --release
    ./scripts/docker_cross_build.sh --r5 --ti-sdk-dir "\$HOME/ti"
    ./scripts/docker_cross_build.sh --clean --both
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --linux)
            TARGET="linux"
            EXPLICIT_TARGET="linux"
            ACTION_REQUESTED="true"
            shift
            ;;
        --r5)
            TARGET="r5"
            EXPLICIT_TARGET="r5"
            ACTION_REQUESTED="true"
            shift
            ;;
        --both)
            TARGET="both"
            EXPLICIT_TARGET="both"
            ACTION_REQUESTED="true"
            shift
            ;;
        --debug)
            BUILD_MODE="debug"
            ACTION_REQUESTED="true"
            shift
            ;;
        --release)
            BUILD_MODE="release"
            ACTION_REQUESTED="true"
            shift
            ;;
        --clean)
            CLEAN_ONLY="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --fetch-sdk)
            FETCH_SDK="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --build-pdk)
            BUILD_PDK="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --setup)
            FETCH_SDK="true"
            BUILD_PDK="true"
            SETUP_ONLY="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --ti-sdk-dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: --ti-sdk-dir requires a value" >&2
                exit 1
            fi
            TI_SDK_DIR="$2"
            ACTION_REQUESTED="true"
            shift 2
            ;;
        --skip-image-build)
            SKIP_IMAGE_BUILD="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Error: Unknown option: $1" >&2
            print_help
            exit 1
            ;;
    esac
done

if [[ "${ACTION_REQUESTED}" != "true" ]]; then
    print_help
    exit 0
fi

if [[ "${BUILD_MODE}" != "debug" && "${BUILD_MODE}" != "release" ]]; then
    echo "Error: BUILD_MODE must be debug or release" >&2
    exit 1
fi

# Setup / fetch / build-pdk are TI-container work. Alone they do not build firmware.
# Explicit --r5 may combine fetch/build-pdk with a firmware build.
if [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" || "${SETUP_ONLY}" == "true" ]]; then
    if [[ "${EXPLICIT_TARGET}" == "linux" ]]; then
        echo "Error: --fetch-sdk, --build-pdk, and --setup are TI/R5 operations and cannot be combined with --linux." >&2
        exit 1
    fi
    if [[ "${EXPLICIT_TARGET}" == "both" ]]; then
        echo "Error: --fetch-sdk, --build-pdk, and --setup cannot be combined with --both. Run --setup first, then --both." >&2
        exit 1
    fi
    if [[ "${EXPLICIT_TARGET}" == "" ]]; then
        TARGET="r5"
        SETUP_ONLY="true"
    elif [[ "${EXPLICIT_TARGET}" == "r5" ]]; then
        TARGET="r5"
        # Explicit --r5: run setup steps then build firmware.
        SETUP_ONLY="false"
    fi
fi

if [[ "${CLEAN_ONLY}" == "true" && -z "${TARGET}" ]]; then
    TARGET="both"
fi

if [[ "${CLEAN_ONLY}" != "true" && "${SETUP_ONLY}" != "true" && -z "${TARGET}" ]]; then
    if [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" ]]; then
        TARGET="r5"
        SETUP_ONLY="true"
    else
        echo "Error: a target is required: --linux, --r5, or --both" >&2
        print_help
        exit 1
    fi
fi

if [[ -z "${TARGET}" ]]; then
    TARGET="r5"
fi

if command -v docker >/dev/null 2>&1; then
    CONTAINER_ENGINE="docker"
    USER_FLAGS=("-u" "$(id -u):$(id -g)")
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
    MOUNT_SUFFIX=":Z"
    USER_FLAGS=("--userns=keep-id")
else
    echo "Error: Neither docker nor podman command found. Install a container engine first." >&2
    exit 1
fi

echo "Using container engine: ${CONTAINER_ENGINE}"

if [[ -d "${REPO_ROOT}/build" && ! -w "${REPO_ROOT}/build" ]]; then
    echo "Error: ${REPO_ROOT}/build is not writable by $(whoami)." >&2
    echo "Fix once on host, then retry: sudo chown -R $(id -u):$(id -g) ${REPO_ROOT}/build" >&2
    exit 1
fi

build_image() {
    local image_name="$1"
    local dockerfile_path="$2"

    if [[ "${SKIP_IMAGE_BUILD}" == "true" ]]; then
        return 0
    fi

    echo "Building image ${image_name} from ${dockerfile_path} ..."
    "${CONTAINER_ENGINE}" build -f "${dockerfile_path}" -t "${image_name}" "${REPO_ROOT}"
}

run_container() {
    local image_name="$1"
    local mount_sdk="$2"
    shift 2
    local -a run_args=(
        run --rm -t
        "${USER_FLAGS[@]}"
        -e HOME=/home/builder
        -v "${REPO_ROOT}:/workspace${MOUNT_SUFFIX}"
        -w /workspace
    )

    if [[ "${mount_sdk}" == "true" ]]; then
        mkdir -p "${TI_SDK_DIR}"
        run_args+=(-v "${TI_SDK_DIR}:/home/builder/ti${MOUNT_SUFFIX}")
    fi

    run_args+=("${image_name}" bash -lc "$*")
    "${CONTAINER_ENGINE}" "${run_args[@]}"
}

check_r5_pdk_libs() {
    local profile="${BUILD_MODE}"
    local sdk_root="${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
    local pdk_path

    if [[ ! -d "${sdk_root}" ]]; then
        echo "Error: TI SDK not found at ${sdk_root}" >&2
        echo "Run: ./scripts/docker_cross_build.sh --fetch-sdk" >&2
        exit 1
    fi

    pdk_path="$(sdk_resolve_pdk_path "${sdk_root}" || true)"
    if [[ -z "${pdk_path}" ]]; then
        echo "Error: pdk_jacinto_* not found under ${sdk_root}" >&2
        echo "Run: ./scripts/docker_cross_build.sh --fetch-sdk" >&2
        exit 1
    fi

    local -a required_libs=(
        "${pdk_path}/packages/ti/csl/lib/j721e/r5f/${profile}/ti.csl.aer5f"
        "${pdk_path}/packages/ti/osal/lib/nonos/j721e/r5f/${profile}/ti.osal.aer5f"
        "${pdk_path}/packages/ti/board/lib/j721e_evm/r5f/${profile}/ti.board.aer5f"
        "${pdk_path}/packages/ti/drv/sciclient/lib/j721e/mcu2_0/${profile}/sciclient.aer5f"
        "${pdk_path}/packages/ti/drv/sciclient/lib/j721e/mcu2_0/${profile}/sciclient_hs.aer5f"
        "${pdk_path}/packages/ti/drv/ipc/lib/j721e/mcu2_0/${profile}/ipc_baremetal.aer5f"
        "${pdk_path}/packages/ti/drv/spi/lib/j721e/r5f/${profile}/ti.drv.spi.aer5f"
        "${pdk_path}/packages/ti/drv/spi/lib/j721e/r5f/${profile}/ti.drv.spi.dma.aer5f"
        "${pdk_path}/packages/ti/drv/uart/lib/j721e/r5f/${profile}/ti.drv.uart.aer5f"
        "${pdk_path}/packages/ti/drv/uart/lib/j721e/r5f/${profile}/ti.drv.uart.dma.aer5f"
        "${pdk_path}/packages/ti/drv/i2c/lib/j721e/r5f/${profile}/ti.drv.i2c.aer5f"
    )

    local missing=0
    for lib in "${required_libs[@]}"; do
        if [[ ! -f "${lib}" ]]; then
            echo "Missing PDK library: ${lib}" >&2
            missing=1
        fi
    done

    if [[ "${missing}" -ne 0 ]]; then
        echo "Error: PDK ${profile} libraries are missing. Run: ./scripts/docker_cross_build.sh --build-pdk" >&2
        exit 1
    fi
}

run_linux_build() {
    build_image "${DEBIAN_IMAGE}" "${DEBIAN_DOCKERFILE}"

    local cmd=""
    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        cmd="make -C /workspace/LINUX_SIDE clean"
        echo "Cleaning Linux build artifacts in Debian 13 container ..."
    else
        cmd="make -C /workspace/LINUX_SIDE clean && make -C /workspace/LINUX_SIDE CROSS_COMPILE=true BUILD_MODE=${BUILD_MODE}"
        echo "Running Linux build in Debian 13 container ..."
        echo "BUILD_MODE=${BUILD_MODE}"
    fi

    run_container "${DEBIAN_IMAGE}" false "${cmd}"
}

run_ti_container() {
    local cmd="$1"
    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"
    run_container "${TI_IMAGE}" true "${cmd}"
}

run_r5_work() {
    local cmd_parts=()

    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        cmd_parts+=("make -C /workspace/R5_SIDE clean")
        echo "Cleaning R5 build artifacts in TI container ..."
        run_ti_container "${cmd_parts[*]}"
        return 0
    fi

    if [[ "${FETCH_SDK}" == "true" ]]; then
        cmd_parts+=("/workspace/scripts/fetch_ti_sdk.sh --ti-sdk-dir /home/builder/ti")
    fi

    if [[ "${BUILD_PDK}" == "true" ]]; then
        # Default build_pdk_libs.sh builds both debug and release profiles.
        cmd_parts+=("/workspace/scripts/build_pdk_libs.sh --ti-sdk-dir /home/builder/ti")
    fi

    if [[ "${SETUP_ONLY}" != "true" && ( "${TARGET}" == "r5" || "${TARGET}" == "both" ) ]]; then
        if [[ "${FETCH_SDK}" != "true" && "${BUILD_PDK}" != "true" ]]; then
            check_r5_pdk_libs
        fi
        cmd_parts+=("make -C /workspace/R5_SIDE clean && make -C /workspace/R5_SIDE BUILD_MODE=${BUILD_MODE}")
        echo "Running R5 firmware build in TI container ..."
        echo "BUILD_MODE=${BUILD_MODE} (PDK profile=${BUILD_MODE})"
    elif [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" ]]; then
        echo "Running TI SDK setup in TI container ..."
    fi

    if [[ ${#cmd_parts[@]} -eq 0 ]]; then
        echo "Error: nothing to run in TI container." >&2
        exit 1
    fi

    local joined=""
    for part in "${cmd_parts[@]}"; do
        if [[ -n "${joined}" ]]; then
            joined+=" && "
        fi
        joined+="${part}"
    done

    run_ti_container "${joined}"
}

case "${TARGET}" in
    linux)
        run_linux_build
        ;;
    r5)
        run_r5_work
        ;;
    both)
        run_linux_build
        run_r5_work
        ;;
    *)
        echo "Error: unknown target: ${TARGET}" >&2
        exit 1
        ;;
esac

if [[ "${CLEAN_ONLY}" == "true" ]]; then
    echo "Clean completed for target=${TARGET}."
elif [[ "${SETUP_ONLY}" == "true" ]]; then
    echo "SDK/PDK setup completed under ${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
    echo "PDK libraries (debug + release) are under that tree; firmware ELFs are produced by --r5 or --both."
elif [[ "${TARGET}" == "linux" ]]; then
    echo "Linux build completed. Artifacts are under ./build (and LINUX_SIDE/build)."
elif [[ "${TARGET}" == "r5" ]]; then
    echo "R5 build completed. Firmware ELF is under ./build/R5_0/"
else
    echo "Container build completed. Artifacts are under ./build/"
fi

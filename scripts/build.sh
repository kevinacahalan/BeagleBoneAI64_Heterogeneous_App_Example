#!/usr/bin/env bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
# shellcheck source=lib/sdk_config.sh
source "${SCRIPT_DIR}/lib/sdk_config.sh"

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

validate_not_root() {
    if [[ "${EUID}" -eq 0 ]]; then
        print_error "Run this script as a regular user, not with sudo."
        return 1
    fi
    return 0
}

print_help() {
    cat <<EOF
Usage: ./scripts/build.sh [options]

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
    - First-time setup: ./scripts/build.sh --setup
    - SDK ${TI_SDK_VERSION} is mounted at /home/builder/ti in the TI container.

Examples:
    ./scripts/build.sh --setup
    ./scripts/build.sh --both
    ./scripts/build.sh --linux --release
    ./scripts/build.sh --r5 --ti-sdk-dir "\$HOME/ti"
    ./scripts/build.sh --clean --both
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
                print_error "--ti-sdk-dir requires a value"
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
            print_error "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

if [[ "${ACTION_REQUESTED}" != "true" ]]; then
    print_header "BeagleBone AI64 build"
    print_info "Project: ${REPO_ROOT}"
    print_info "All builds run inside Podman/Docker."
    print_info "First-time R5 setup: ./scripts/build.sh --setup"
    echo
    print_help
    exit 0
fi

validate_not_root || exit 1

if [[ "${BUILD_MODE}" != "debug" && "${BUILD_MODE}" != "release" ]]; then
    print_error "BUILD_MODE must be debug or release"
    exit 1
fi

# Setup / fetch / build-pdk are TI-container work. Alone they do not build firmware.
# Explicit --r5 may combine fetch/build-pdk with a firmware build.
if [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" || "${SETUP_ONLY}" == "true" ]]; then
    if [[ "${EXPLICIT_TARGET}" == "linux" ]]; then
        print_error "--fetch-sdk, --build-pdk, and --setup are TI/R5 operations and cannot be combined with --linux."
        exit 1
    fi
    if [[ "${EXPLICIT_TARGET}" == "both" ]]; then
        print_error "--fetch-sdk, --build-pdk, and --setup cannot be combined with --both. Run --setup first, then --both."
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
        print_error "A target is required: --linux, --r5, or --both"
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
    print_error "Neither docker nor podman command found. Install a container engine first."
    exit 1
fi

print_info "Using container engine: ${CONTAINER_ENGINE}"

if [[ -d "${REPO_ROOT}/build" && ! -w "${REPO_ROOT}/build" ]]; then
    print_error "${REPO_ROOT}/build is not writable by $(whoami)."
    print_error "Fix once on host, then retry: sudo chown -R $(id -u):$(id -g) ${REPO_ROOT}/build"
    exit 1
fi

build_image() {
    local image_name="$1"
    local dockerfile_path="$2"

    if [[ "${SKIP_IMAGE_BUILD}" == "true" ]]; then
        return 0
    fi

    print_info "Building image ${image_name} from ${dockerfile_path} ..."
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
        print_error "TI SDK not found at ${sdk_root}"
        print_error "Run: ./scripts/build.sh --fetch-sdk"
        exit 1
    fi

    pdk_path="$(sdk_resolve_pdk_path "${sdk_root}" || true)"
    if [[ -z "${pdk_path}" ]]; then
        print_error "pdk_jacinto_* not found under ${sdk_root}"
        print_error "Run: ./scripts/build.sh --fetch-sdk"
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
            print_error "Missing PDK library: ${lib}"
            missing=1
        fi
    done

    if [[ "${missing}" -ne 0 ]]; then
        print_error "PDK ${profile} libraries are missing. Run: ./scripts/build.sh --build-pdk"
        exit 1
    fi
}

run_linux_build() {
    build_image "${DEBIAN_IMAGE}" "${DEBIAN_DOCKERFILE}"

    local cmd=""
    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        cmd="make -C /workspace/LINUX_SIDE clean"
        print_header "Cleaning Linux build artifacts"
    else
        cmd="make -C /workspace/LINUX_SIDE clean && make -C /workspace/LINUX_SIDE CROSS_COMPILE=true BUILD_MODE=${BUILD_MODE}"
        print_header "Linux build (${BUILD_MODE})"
        print_info "Debian 13 container"
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
        print_header "Cleaning R5 build artifacts"
        run_ti_container "${cmd_parts[*]}"
        return 0
    fi

    if [[ "${FETCH_SDK}" == "true" ]]; then
        cmd_parts+=("/workspace/scripts/lib/fetch_ti_sdk.sh --ti-sdk-dir /home/builder/ti")
    fi

    if [[ "${BUILD_PDK}" == "true" ]]; then
        # Default build_pdk_libs.sh builds both debug and release profiles.
        cmd_parts+=("/workspace/scripts/lib/build_pdk_libs.sh --ti-sdk-dir /home/builder/ti")
    fi

    if [[ "${SETUP_ONLY}" != "true" && ( "${TARGET}" == "r5" || "${TARGET}" == "both" ) ]]; then
        if [[ "${FETCH_SDK}" != "true" && "${BUILD_PDK}" != "true" ]]; then
            check_r5_pdk_libs
        fi
        cmd_parts+=("make -C /workspace/R5_SIDE clean && make -C /workspace/R5_SIDE BUILD_MODE=${BUILD_MODE}")
        print_header "R5 firmware build (${BUILD_MODE})"
        print_info "PDK profile=${BUILD_MODE}"
    elif [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" ]]; then
        print_header "TI SDK / PDK setup"
    fi

    if [[ ${#cmd_parts[@]} -eq 0 ]]; then
        print_error "Nothing to run in TI container."
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
        print_error "Unknown target: ${TARGET}"
        exit 1
        ;;
esac

if [[ "${CLEAN_ONLY}" == "true" ]]; then
    print_success "Clean completed for target=${TARGET}."
elif [[ "${SETUP_ONLY}" == "true" ]]; then
    print_success "SDK/PDK setup completed under ${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
    print_info "PDK libraries (debug + release) are under that tree; firmware ELFs are produced by --r5 or --both."
elif [[ "${TARGET}" == "linux" ]]; then
    print_success "Linux build completed. Artifacts are under ./build (and LINUX_SIDE/build)."
elif [[ "${TARGET}" == "r5" ]]; then
    print_success "R5 build completed. Firmware ELF is under ./build/R5_0/"
else
    print_success "Build completed. Artifacts are under ./build/"
fi

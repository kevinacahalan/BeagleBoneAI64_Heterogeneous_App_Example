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
# shellcheck source=lib/pssp_config.sh
source "${SCRIPT_DIR}/lib/pssp_config.sh"

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
FETCH_PSSP="false"
BUILD_PSSP="false"
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

want_ti_setup() {
    [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" \
        || "${FETCH_PSSP}" == "true" || "${BUILD_PSSP}" == "true" ]]
}

print_help() {
    cat <<EOF
Usage: ./scripts/build.sh [options]

Build inside containers (Podman or Docker):
  - Debian 13 image: Linux aarch64 cross-build (gpiod v2)
  - TI ubuntu-distro image: R5 firmware, PRU firmware, SDK/PDK, PSSP

Run with no arguments to show this help.

Firmware targets:
    --linux                Build only the Linux side (aarch64).
    --r5                   Build only the R5 firmware (requires --setup first).
    --pru                  Build only PRU0_0 firmware (requires --setup first).
    --all                  Build Linux + R5 + PRU (requires --setup first).

Build mode:
    --debug                Debug build (default): app flags + PDK debug libs.
    --release              Release build: app flags + PDK release libs.
    --clean                Clean build artifacts for the selected target(s) and exit.

Dependency setup (TI container; same idea for R5 and PRU):
    --setup                Fetch/build TI SDK+PDK and PSSP+rpmsg_lib.
    --fetch-sdk            Download/extract TI SDK ${TI_SDK_VERSION}.
    --build-pdk            Build PDK debug and release libraries.
    --fetch-pssp           Clone PSSP into ${PSSP_DIR_REL} (pinned commit).
    --build-pssp           Build PSSP lib/rpmsg_lib.lib with clpru.
    --ti-sdk-dir <path>    Host path for TI SDK. Default: \$HOME/ti

Other:
    --skip-image-build     Reuse existing images; skip docker/podman build.
    -h, --help             Show this help.

Notes:
    - First-time: ./scripts/build.sh --setup
    - Then:       ./scripts/build.sh --r5 / --pru / --linux / --all
    - Firmware targets only compile project code; they do not fetch deps.
    - You may combine setup flags with a firmware target, e.g. --r5 --setup.
    - SDK ${TI_SDK_VERSION} is mounted at /home/builder/ti in the TI container.

Examples:
    ./scripts/build.sh --setup
    ./scripts/build.sh --all
    ./scripts/build.sh --linux --release
    ./scripts/build.sh --r5 --ti-sdk-dir "\$HOME/ti"
    ./scripts/build.sh --pru
    ./scripts/build.sh --clean --all
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
        --pru)
            TARGET="pru"
            EXPLICIT_TARGET="pru"
            ACTION_REQUESTED="true"
            shift
            ;;
        --all)
            TARGET="all"
            EXPLICIT_TARGET="all"
            ACTION_REQUESTED="true"
            shift
            ;;
        --both)
            print_warning "--both is deprecated; use --all (Linux + R5 + PRU)."
            TARGET="all"
            EXPLICIT_TARGET="all"
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
        --fetch-pssp)
            FETCH_PSSP="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --build-pssp)
            BUILD_PSSP="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --setup)
            FETCH_SDK="true"
            BUILD_PDK="true"
            FETCH_PSSP="true"
            BUILD_PSSP="true"
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
    print_info "First-time setup: ./scripts/build.sh --setup"
    echo
    print_help
    exit 0
fi

validate_not_root || exit 1

if [[ "${BUILD_MODE}" != "debug" && "${BUILD_MODE}" != "release" ]]; then
    print_error "BUILD_MODE must be debug or release"
    exit 1
fi

# Setup flags alone (no firmware target) → dependency setup only.
# Setup flags + --r5/--pru/--all → setup then that firmware target.
if want_ti_setup || [[ "${SETUP_ONLY}" == "true" ]]; then
    if [[ "${EXPLICIT_TARGET}" == "linux" ]]; then
        print_error "Dependency setup flags cannot be combined with --linux."
        exit 1
    fi
    if [[ "${EXPLICIT_TARGET}" == "" ]]; then
        SETUP_ONLY="true"
        TARGET="setup"
    elif [[ "${EXPLICIT_TARGET}" == "r5" || "${EXPLICIT_TARGET}" == "pru" || "${EXPLICIT_TARGET}" == "all" ]]; then
        TARGET="${EXPLICIT_TARGET}"
        SETUP_ONLY="false"
    fi
fi

if [[ "${CLEAN_ONLY}" == "true" && -z "${TARGET}" ]]; then
    TARGET="all"
fi

if [[ "${CLEAN_ONLY}" != "true" && "${SETUP_ONLY}" != "true" && -z "${TARGET}" ]]; then
    if want_ti_setup; then
        TARGET="setup"
        SETUP_ONLY="true"
    else
        print_error "A target is required: --linux, --r5, --pru, or --all"
        print_help
        exit 1
    fi
fi

if [[ -z "${TARGET}" ]]; then
    TARGET="setup"
fi

if command -v docker >/dev/null 2>&1; then
    CONTAINER_ENGINE="docker"
    # Match host UID/GID so bind-mounted outputs are owned by the invoking user.
    USER_FLAGS=("-u" "$(id -u):$(id -g)")
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
    MOUNT_SUFFIX=":Z"
    # keep-id alone still starts as image USER (root in Dockerfile.ti), which
    # creates bind-mount files as nobody/overflow. Always pass -u with keep-id.
    USER_FLAGS=("--userns=keep-id" "-u" "$(id -u):$(id -g)")
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

join_commands() {
    local joined=""
    local part
    for part in "$@"; do
        if [[ -z "${part}" ]]; then
            continue
        fi
        if [[ -n "${joined}" ]]; then
            joined+=" && "
        fi
        joined+="${part}"
    done
    printf '%s' "${joined}"
}

check_r5_pdk_libs() {
    local profile="${BUILD_MODE}"
    local sdk_root="${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
    local pdk_path

    if [[ ! -d "${sdk_root}" ]]; then
        print_error "TI SDK not found at ${sdk_root}"
        print_error "Run: ./scripts/build.sh --setup"
        exit 1
    fi

    pdk_path="$(sdk_resolve_pdk_path "${sdk_root}" || true)"
    if [[ -z "${pdk_path}" ]]; then
        print_error "pdk_jacinto_* not found under ${sdk_root}"
        print_error "Run: ./scripts/build.sh --setup"
        exit 1
    fi

    local -a required_libs=(
        "${pdk_path}/packages/ti/csl/lib/j721e/r5f/${profile}/ti.csl.aer5f"
        "${pdk_path}/packages/ti/osal/lib/nonos/j721e/r5f/${profile}/ti.osal.aer5f"
        "${pdk_path}/packages/ti/board/lib/j721e_evm/r5f/${profile}/ti.board.aer5f"
        # TI only builds sciclient/IPC for mcu2_0 under release/ (no debug/ tree).
        "${pdk_path}/packages/ti/drv/sciclient/lib/j721e/mcu2_0/release/sciclient.aer5f"
        "${pdk_path}/packages/ti/drv/sciclient/lib/j721e/mcu2_0/release/sciclient_hs.aer5f"
        "${pdk_path}/packages/ti/drv/ipc/lib/j721e/mcu2_0/release/ipc_baremetal.aer5f"
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
        print_error "PDK ${profile} libraries are missing. Run: ./scripts/build.sh --setup"
        exit 1
    fi
}

check_pssp_ready() {
    local pssp_dir
    pssp_dir="$(pssp_resolve_dir "${REPO_ROOT}")"

    if ! pssp_headers_ok "${pssp_dir}"; then
        print_error "PSSP headers not found under ${pssp_dir}"
        print_error "Run: ./scripts/build.sh --setup"
        exit 1
    fi
    if ! pssp_lib_ok "${pssp_dir}"; then
        print_error "PSSP lib/rpmsg_lib.lib missing under ${pssp_dir}"
        print_error "Run: ./scripts/build.sh --setup"
        exit 1
    fi
}

# Fetch/build TI SDK, PDK, and/or PSSP in one TI-container invocation.
run_ti_dep_setup() {
    local -a cmd_parts=()
    local need_sdk_mount="false"
    local joined=""

    if [[ "${FETCH_SDK}" != "true" && "${BUILD_PDK}" != "true" \
        && "${FETCH_PSSP}" != "true" && "${BUILD_PSSP}" != "true" ]]; then
        return 0
    fi

    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"

    if [[ "${FETCH_SDK}" == "true" || "${BUILD_PDK}" == "true" ]]; then
        need_sdk_mount="true"
    fi

    print_header "TI dependency setup"
    if [[ "${FETCH_SDK}" == "true" ]]; then
        print_info "Fetch TI SDK ${TI_SDK_VERSION}"
        cmd_parts+=("/workspace/scripts/lib/fetch_ti_sdk.sh --ti-sdk-dir /home/builder/ti")
    fi
    if [[ "${BUILD_PDK}" == "true" ]]; then
        print_info "Build PDK libraries (debug + release)"
        cmd_parts+=("/workspace/scripts/lib/build_pdk_libs.sh --ti-sdk-dir /home/builder/ti")
    fi
    if [[ "${FETCH_PSSP}" == "true" ]]; then
        print_info "Fetch PSSP (${PSSP_COMMIT})"
        cmd_parts+=("/workspace/scripts/lib/fetch_pssp.sh --repo-root /workspace")
    fi
    if [[ "${BUILD_PSSP}" == "true" ]]; then
        print_info "Build PSSP rpmsg_lib"
        cmd_parts+=("/workspace/scripts/lib/build_pssp_lib.sh --repo-root /workspace")
    fi

    joined="$(join_commands "${cmd_parts[@]}")"
    run_container "${TI_IMAGE}" "${need_sdk_mount}" "${joined}"
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

run_r5_firmware() {
    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"

    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        print_header "Cleaning R5 build artifacts"
        run_container "${TI_IMAGE}" true "make -C /workspace/R5_SIDE clean"
        return 0
    fi

    # Skip the host-side check when this same invocation just built the PDK.
    if [[ "${BUILD_PDK}" != "true" && "${FETCH_SDK}" != "true" ]]; then
        check_r5_pdk_libs
    fi

    print_header "R5 firmware build (${BUILD_MODE})"
    print_info "PDK profile=${BUILD_MODE}"
    run_container "${TI_IMAGE}" true \
        "make -C /workspace/R5_SIDE clean && make -C /workspace/R5_SIDE BUILD_MODE=${BUILD_MODE}"
}

run_pru_firmware() {
    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"

    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        print_header "Cleaning PRU0_0 build artifacts"
        run_container "${TI_IMAGE}" false "make -C /workspace/PRU0_0_SIDE clean"
        return 0
    fi

    # Skip the host-side check when this same invocation just built PSSP.
    if [[ "${BUILD_PSSP}" != "true" && "${FETCH_PSSP}" != "true" ]]; then
        check_pssp_ready
    fi

    print_header "PRU0_0 firmware build"
    print_info "TI container (clpru / lnkpru)"
    run_container "${TI_IMAGE}" false \
        "make -C /workspace/PRU0_0_SIDE clean && make -C /workspace/PRU0_0_SIDE all"
}

case "${TARGET}" in
    setup)
        run_ti_dep_setup
        ;;
    linux)
        run_linux_build
        ;;
    r5)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_r5_firmware
        ;;
    pru)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_pru_firmware
        ;;
    all)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_linux_build
        run_r5_firmware
        run_pru_firmware
        ;;
    *)
        print_error "Unknown target: ${TARGET}"
        exit 1
        ;;
esac

if [[ "${CLEAN_ONLY}" == "true" ]]; then
    print_success "Clean completed for target=${TARGET}."
elif [[ "${TARGET}" == "setup" || "${SETUP_ONLY}" == "true" ]]; then
    print_success "Dependency setup completed (TI SDK/PDK + PSSP)."
    print_info "SDK/PDK: ${TI_SDK_DIR}/${TI_SDK_ROOT_NAME}"
    print_info "PSSP:    ${REPO_ROOT}/${PSSP_DIR_REL}"
    print_info "Next: ./scripts/build.sh --r5 | --pru | --linux | --all"
elif [[ "${TARGET}" == "linux" ]]; then
    print_success "Linux build completed. Artifacts are under ./build (and LINUX_SIDE/build)."
elif [[ "${TARGET}" == "r5" ]]; then
    print_success "R5 build completed. Firmware ELF is under ./build/R5_0/"
elif [[ "${TARGET}" == "pru" ]]; then
    print_success "PRU0_0 build completed. Firmware is under ./build/PRU0_0/"
elif [[ "${TARGET}" == "all" ]]; then
    print_success "Full build completed (Linux + R5 + PRU). Artifacts under ./build/"
else
    print_success "Build completed. Artifacts are under ./build/"
fi

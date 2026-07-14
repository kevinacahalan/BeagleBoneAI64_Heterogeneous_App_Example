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
EXTRA_NAME=""
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

# Side filters (sub-options). If none are set, build all sides for the primary target.
SIDE_LINUX="false"
SIDE_R5="false"
SIDE_PRU="false"
SIDE_RTU="false"

# Extra-example demo names under extra-examples/
EXTRA_DEMOS=(pru0_0-hello pru0_0-rpmsg-led rtu0_0-pru0_0-rpmsg-led)

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

any_side_filter() {
    [[ "${SIDE_LINUX}" == "true" || "${SIDE_R5}" == "true" \
        || "${SIDE_PRU}" == "true" || "${SIDE_RTU}" == "true" ]]
}

# True if no side filters were given, or the named side was requested.
want_side() {
    local side="$1"
    if ! any_side_filter; then
        return 0
    fi
    case "${side}" in
        linux) [[ "${SIDE_LINUX}" == "true" ]] ;;
        r5)    [[ "${SIDE_R5}" == "true" ]] ;;
        pru)   [[ "${SIDE_PRU}" == "true" ]] ;;
        rtu)   [[ "${SIDE_RTU}" == "true" ]] ;;
        *)     return 1 ;;
    esac
}

print_help() {
    cat <<EOF
Usage: ./scripts/build.sh [options]

Build inside containers (Podman or Docker):
  - Debian 13 image: Linux aarch64 cross-build (gpiod v2)
  - TI ubuntu-distro image: R5 + extra-examples PRU/RTU firmware, SDK/PDK, PSSP

Run with no arguments to show this help.

Primary targets:
    --main                 Main demo (Linux ↔ R5F0_0). Default sides: --linux --r5
    --extras               All demos under extra-examples/. Default: all sides present
    --extra <name>         One extra: pru0_0-hello | pru0_0-rpmsg-led | rtu0_0-pru0_0-rpmsg-led
    --all                  Main demo + all extras (requires --setup first for firmware)

Side filters (optional sub-options; omit to build every side for the target):
    --linux                Linux / host side
    --r5                   R5 firmware side
    --pru                  PRU0_0 firmware side
    --rtu                  RTU0_0 firmware side

Build mode:
    --debug                Debug build (default): app flags + PDK debug libs.
    --release              Release build: app flags + PDK release libs.
    --clean                Clean build artifacts for the selected target(s) and exit.

Dependency setup (TI container):
    --setup                Fetch/build TI SDK+PDK and PSSP+rpmsg_lib.
    --fetch-sdk            Download/extract TI SDK ${TI_SDK_VERSION}.
    --build-pdk            Build PDK debug and release libraries.
    --fetch-pssp           Clone PSSP into \$TI_SDK_DIR/${PSSP_DIR_NAME} (pinned commit).
    --build-pssp           Build PSSP lib/rpmsg_lib.lib with clpru.
    --ti-sdk-dir <path>    Host path for TI SDK. Default: \$HOME/ti

Other:
    --skip-image-build     Reuse existing images; skip docker/podman build.
    -h, --help             Show this help.

Notes:
    - First-time: ./scripts/build.sh --setup
    - Side filters require a primary target (--main / --extras / --extra / --all).
    - Firmware targets only compile project code; they do not fetch deps.
    - You may combine setup flags with a firmware target, e.g. --main --setup.
    - SDK ${TI_SDK_VERSION} is mounted at /home/builder/ti in the TI container.

Examples:
    ./scripts/build.sh --setup
    ./scripts/build.sh --main
    ./scripts/build.sh --main --r5
    ./scripts/build.sh --main --linux
    ./scripts/build.sh --extras
    ./scripts/build.sh --extras --pru
    ./scripts/build.sh --extra pru0_0-rpmsg-led --pru
    ./scripts/build.sh --extra rtu0_0-pru0_0-rpmsg-led --rtu
    ./scripts/build.sh --all
    ./scripts/build.sh --all --pru
    ./scripts/build.sh --clean --all
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --linux)
            SIDE_LINUX="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --r5)
            SIDE_R5="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --pru)
            SIDE_PRU="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --rtu)
            SIDE_RTU="true"
            ACTION_REQUESTED="true"
            shift
            ;;
        --extras)
            TARGET="extras"
            EXPLICIT_TARGET="extras"
            ACTION_REQUESTED="true"
            shift
            ;;
        --extra)
            if [[ $# -lt 2 ]]; then
                print_error "--extra requires a demo name: ${EXTRA_DEMOS[*]}"
                exit 1
            fi
            EXTRA_NAME="$2"
            TARGET="extra"
            EXPLICIT_TARGET="extra"
            ACTION_REQUESTED="true"
            shift 2
            ;;
        --main)
            TARGET="main"
            EXPLICIT_TARGET="main"
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
            print_warning "--both is deprecated; use --all (main demo + extras)."
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

# Setup flags alone (no primary target) → dependency setup only.
# Setup flags + --main/--extras/--extra/--all → setup then that target.
if want_ti_setup || [[ "${SETUP_ONLY}" == "true" ]]; then
    if [[ "${EXPLICIT_TARGET}" == "" ]]; then
        if any_side_filter; then
            print_error "Side filters (--linux/--r5/--pru/--rtu) require a primary target."
            print_error "Example: ./scripts/build.sh --main --r5"
            exit 1
        fi
        SETUP_ONLY="true"
        TARGET="setup"
    elif [[ "${EXPLICIT_TARGET}" == "main" || "${EXPLICIT_TARGET}" == "extras" \
            || "${EXPLICIT_TARGET}" == "extra" || "${EXPLICIT_TARGET}" == "all" ]]; then
        TARGET="${EXPLICIT_TARGET}"
        SETUP_ONLY="false"
    fi
fi

if [[ "${CLEAN_ONLY}" == "true" && -z "${TARGET}" ]]; then
    if any_side_filter; then
        print_error "Side filters require a primary target with --clean."
        print_error "Example: ./scripts/build.sh --clean --main --r5"
        exit 1
    fi
    TARGET="all"
fi

if [[ "${CLEAN_ONLY}" != "true" && "${SETUP_ONLY}" != "true" && -z "${TARGET}" ]]; then
    if want_ti_setup; then
        TARGET="setup"
        SETUP_ONLY="true"
    elif any_side_filter; then
        print_error "Side filters (--linux/--r5/--pru/--rtu) require a primary target."
        print_error "Example: ./scripts/build.sh --main --linux"
        exit 1
    else
        print_error "A primary target is required: --main, --extras, --extra <name>, or --all"
        print_help
        exit 1
    fi
fi

if [[ -z "${TARGET}" ]]; then
    TARGET="setup"
fi

if [[ "${TARGET}" == "extra" ]]; then
    valid_extra="false"
    for d in "${EXTRA_DEMOS[@]}"; do
        if [[ "${EXTRA_NAME}" == "${d}" ]]; then
            valid_extra="true"
            break
        fi
    done
    if [[ "${valid_extra}" != "true" ]]; then
        print_error "Unknown --extra name: ${EXTRA_NAME}"
        print_error "Valid: ${EXTRA_DEMOS[*]}"
        exit 1
    fi
fi

# Reject side filters that do not apply to the chosen primary target.
if any_side_filter; then
    case "${TARGET}" in
        main)
            if [[ "${SIDE_PRU}" == "true" || "${SIDE_RTU}" == "true" ]]; then
                print_error "--main only supports side filters --linux and/or --r5."
                exit 1
            fi
            ;;
        extras|extra)
            if [[ "${SIDE_LINUX}" == "true" || "${SIDE_R5}" == "true" ]]; then
                print_error "--extras/--extra only support side filters --pru and/or --rtu."
                exit 1
            fi
            ;;
        all)
            # all sides are valid under --all
            ;;
        setup)
            print_error "Side filters cannot be used with dependency setup alone."
            exit 1
            ;;
    esac
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
    pssp_dir="$(pssp_resolve_dir "${TI_SDK_DIR}")"

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
    local joined=""

    if [[ "${FETCH_SDK}" != "true" && "${BUILD_PDK}" != "true" \
        && "${FETCH_PSSP}" != "true" && "${BUILD_PSSP}" != "true" ]]; then
        return 0
    fi

    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"

    # SDK, PDK, and PSSP all live under the mounted TI install dir (~/ti).
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
        cmd_parts+=("/workspace/scripts/lib/fetch_pssp.sh --ti-sdk-dir /home/builder/ti")
    fi
    if [[ "${BUILD_PSSP}" == "true" ]]; then
        print_info "Build PSSP rpmsg_lib"
        cmd_parts+=("/workspace/scripts/lib/build_pssp_lib.sh --ti-sdk-dir /home/builder/ti")
    fi

    joined="$(join_commands "${cmd_parts[@]}")"
    run_container "${TI_IMAGE}" true "${joined}"
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

run_extra_demo() {
    local name="$1"
    local pssp_path="/home/builder/ti/${PSSP_DIR_NAME}"
    local -a cmd_parts=()
    local cmd=""
    local do_pru="false"
    local do_rtu="false"

    if want_side pru; then
        do_pru="true"
    fi
    if want_side rtu; then
        do_rtu="true"
    fi

    case "${name}" in
        pru0_0-hello)
            if [[ "${do_pru}" != "true" ]]; then
                print_info "Skipping ${name} (no --pru side selected)"
                return 0
            fi
            if [[ "${CLEAN_ONLY}" == "true" ]]; then
                cmd_parts+=("make -C /workspace/extra-examples/pru0_0-hello/PRU0_0_SIDE clean")
            else
                cmd_parts+=("make -C /workspace/extra-examples/pru0_0-hello/PRU0_0_SIDE clean && make -C /workspace/extra-examples/pru0_0-hello/PRU0_0_SIDE all PSSP=${pssp_path}")
            fi
            ;;
        pru0_0-rpmsg-led)
            if [[ "${do_pru}" != "true" ]]; then
                print_info "Skipping ${name} (no --pru side selected)"
                return 0
            fi
            if [[ "${CLEAN_ONLY}" == "true" ]]; then
                cmd_parts+=("make -C /workspace/extra-examples/pru0_0-rpmsg-led/PRU0_0_SIDE clean")
            else
                cmd_parts+=("make -C /workspace/extra-examples/pru0_0-rpmsg-led/PRU0_0_SIDE clean && make -C /workspace/extra-examples/pru0_0-rpmsg-led/PRU0_0_SIDE all PSSP=${pssp_path}")
            fi
            ;;
        rtu0_0-pru0_0-rpmsg-led)
            if [[ "${do_pru}" != "true" && "${do_rtu}" != "true" ]]; then
                print_info "Skipping ${name} (need --pru and/or --rtu)"
                return 0
            fi
            if [[ "${CLEAN_ONLY}" == "true" ]]; then
                if [[ "${do_pru}" == "true" ]]; then
                    cmd_parts+=("make -C /workspace/extra-examples/rtu0_0-pru0_0-rpmsg-led/PRU0_0_SIDE clean")
                fi
                if [[ "${do_rtu}" == "true" ]]; then
                    cmd_parts+=("make -C /workspace/extra-examples/rtu0_0-pru0_0-rpmsg-led/RTU0_0_SIDE clean")
                fi
                if [[ "${do_pru}" == "true" && "${do_rtu}" == "true" ]]; then
                    cmd_parts+=("rm -rf /workspace/build/extra-examples/rtu0_0-pru0_0-rpmsg-led")
                fi
            else
                if [[ "${do_pru}" == "true" ]]; then
                    cmd_parts+=("make -C /workspace/extra-examples/rtu0_0-pru0_0-rpmsg-led/PRU0_0_SIDE clean && make -C /workspace/extra-examples/rtu0_0-pru0_0-rpmsg-led/PRU0_0_SIDE all PSSP=${pssp_path}")
                fi
                if [[ "${do_rtu}" == "true" ]]; then
                    cmd_parts+=("make -C /workspace/extra-examples/rtu0_0-pru0_0-rpmsg-led/RTU0_0_SIDE clean && make -C /workspace/extra-examples/rtu0_0-pru0_0-rpmsg-led/RTU0_0_SIDE all PSSP=${pssp_path}")
                fi
            fi
            ;;
        *)
            print_error "Unknown extra demo: ${name}"
            exit 1
            ;;
    esac

    if [[ ${#cmd_parts[@]} -eq 0 ]]; then
        return 0
    fi

    cmd="$(join_commands "${cmd_parts[@]}")"
    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        run_container "${TI_IMAGE}" false "${cmd}"
    else
        run_container "${TI_IMAGE}" true "${cmd}"
    fi
}

run_extras_firmware() {
    # Nothing to do if only main-demo sides were requested under --all.
    if any_side_filter && ! want_side pru && ! want_side rtu; then
        print_info "Skipping extra-examples (no --pru/--rtu side selected)"
        return 0
    fi

    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"

    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        print_header "Cleaning extra-examples build artifacts"
        local name
        for name in "${EXTRA_DEMOS[@]}"; do
            run_extra_demo "${name}"
        done
        return 0
    fi

    # Skip the host-side check when this same invocation just built PSSP.
    if [[ "${BUILD_PSSP}" != "true" && "${FETCH_PSSP}" != "true" ]]; then
        check_pssp_ready
    fi

    print_header "extra-examples firmware build"
    print_info "TI container (clpru / lnkpru); PSSP from /home/builder/ti/${PSSP_DIR_NAME}"
    local name
    for name in "${EXTRA_DEMOS[@]}"; do
        print_info "Building ${name}..."
        run_extra_demo "${name}"
    done
}

run_one_extra() {
    if any_side_filter && ! want_side pru && ! want_side rtu; then
        print_error "--extra only supports side filters --pru and/or --rtu."
        exit 1
    fi

    build_image "${TI_IMAGE}" "${TI_DOCKERFILE}"

    if [[ "${CLEAN_ONLY}" == "true" ]]; then
        print_header "Cleaning extra-example ${EXTRA_NAME}"
        run_extra_demo "${EXTRA_NAME}"
        return 0
    fi

    if [[ "${BUILD_PSSP}" != "true" && "${FETCH_PSSP}" != "true" ]]; then
        check_pssp_ready
    fi

    print_header "extra-example build: ${EXTRA_NAME}"
    print_info "TI container (clpru / lnkpru); PSSP from /home/builder/ti/${PSSP_DIR_NAME}"
    run_extra_demo "${EXTRA_NAME}"
}

run_main_demo() {
    local did_something="false"

    if want_side linux; then
        run_linux_build
        did_something="true"
    fi
    if want_side r5; then
        run_r5_firmware
        did_something="true"
    fi
    if [[ "${did_something}" != "true" ]]; then
        print_info "Skipping main demo (no --linux/--r5 side selected)"
    fi
}

case "${TARGET}" in
    setup)
        run_ti_dep_setup
        ;;
    extras)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_extras_firmware
        ;;
    extra)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_one_extra
        ;;
    main)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_main_demo
        ;;
    all)
        if [[ "${CLEAN_ONLY}" != "true" ]] && want_ti_setup; then
            run_ti_dep_setup
        fi
        run_main_demo
        run_extras_firmware
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
    print_info "PSSP:    ${TI_SDK_DIR}/${PSSP_DIR_NAME}"
    print_info "Next: ./scripts/build.sh --main | --extras | --all"
elif [[ "${TARGET}" == "extras" ]]; then
    print_success "Extra examples built. Firmware under ./build/extra-examples/"
elif [[ "${TARGET}" == "extra" ]]; then
    print_success "Extra example ${EXTRA_NAME} built under ./build/extra-examples/${EXTRA_NAME}/"
elif [[ "${TARGET}" == "main" ]]; then
    print_success "Main demo build completed. Artifacts under ./build/"
elif [[ "${TARGET}" == "all" ]]; then
    print_success "Full build completed (main demo + extras). Artifacts under ./build/"
else
    print_success "Build completed. Artifacts are under ./build/"
fi

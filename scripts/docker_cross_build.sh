#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

IMAGE_NAME="localhost/debian13-bbai64-build:latest"
DOCKERFILE_PATH="${REPO_ROOT}/docker/Dockerfile.debian13"
TARGET="both"
TI_SDK_DIR="${HOME}/ti"
SKIP_IMAGE_BUILD="false"
CONTAINER_ENGINE=""
MOUNT_SUFFIX=""
USER_FLAGS=()

print_help() {
    cat <<'EOF'
Usage: ./scripts/docker_cross_build.sh [options]

Options:
  --linux                Build only LINUX_SIDE for BeagleBone (aarch64).
  --r5                   Build only R5_SIDE firmware.
  --both                 Build both targets (default).
  --ti-sdk-dir <path>    Host path containing TI SDK folder(s). Default: $HOME/ti
    --image <name:tag>     Docker image name. Default: localhost/debian13-bbai64-build:latest
  --skip-image-build     Reuse existing image and skip docker build.
  -h, --help             Show this help.

Examples:
  ./scripts/docker_cross_build.sh --both
  ./scripts/docker_cross_build.sh --linux
  ./scripts/docker_cross_build.sh --r5 --ti-sdk-dir "$HOME/ti"
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
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
        --ti-sdk-dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: --ti-sdk-dir requires a value"
                exit 1
            fi
            TI_SDK_DIR="$2"
            shift 2
            ;;
        --image)
            if [[ $# -lt 2 ]]; then
                echo "Error: --image requires a value"
                exit 1
            fi
            IMAGE_NAME="$2"
            shift 2
            ;;
        --skip-image-build)
            SKIP_IMAGE_BUILD="true"
            shift
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Error: Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

if command -v docker >/dev/null 2>&1; then
    CONTAINER_ENGINE="docker"
    USER_FLAGS=("-u" "$(id -u):$(id -g)")
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_ENGINE="podman"
    MOUNT_SUFFIX=":Z"
    USER_FLAGS=("--userns=keep-id")
else
    echo "Error: Neither docker nor podman command found. Install a container engine first."
    exit 1
fi

echo "Using container engine: ${CONTAINER_ENGINE}"

if [[ -d "${REPO_ROOT}/build" && ! -w "${REPO_ROOT}/build" ]]; then
    echo "Error: ${REPO_ROOT}/build is not writable by $(whoami)."
    echo "Fix once on host, then retry: sudo chown -R $(id -u):$(id -g) ${REPO_ROOT}/build"
    exit 1
fi

if [[ "${SKIP_IMAGE_BUILD}" != "true" ]]; then
    echo "Building Docker image ${IMAGE_NAME} from ${DOCKERFILE_PATH}..."
    "${CONTAINER_ENGINE}" build -f "${DOCKERFILE_PATH}" -t "${IMAGE_NAME}" "${REPO_ROOT}"
fi

CONTAINER_CMD=""

if [[ "${TARGET}" == "linux" || "${TARGET}" == "both" ]]; then
    CONTAINER_CMD+="make -C /workspace/LINUX_SIDE clean && "
    CONTAINER_CMD+="make -C /workspace/LINUX_SIDE CROSS_COMPILE=true"
fi

if [[ "${TARGET}" == "r5" || "${TARGET}" == "both" ]]; then
    if [[ ! -d "${TI_SDK_DIR}" ]]; then
        echo "Error: TI SDK directory not found: ${TI_SDK_DIR}"
        echo "Pass a valid path with --ti-sdk-dir <path>."
        exit 1
    fi

    if [[ -n "${CONTAINER_CMD}" ]]; then
        CONTAINER_CMD+=" && "
    fi

    CONTAINER_CMD+="make -C /workspace/R5_SIDE clean && "
    CONTAINER_CMD+="make -C /workspace/R5_SIDE"
fi

if [[ -z "${CONTAINER_CMD}" ]]; then
    echo "Error: nothing to build."
    exit 1
fi

echo "Running ${TARGET} build in container..."
"${CONTAINER_ENGINE}" run --rm -t \
    "${USER_FLAGS[@]}" \
    -e HOME=/home/builder \
    -v "${REPO_ROOT}:/workspace${MOUNT_SUFFIX}" \
    -v "${TI_SDK_DIR}:/home/builder/ti${MOUNT_SUFFIX}" \
    -w /workspace \
    "${IMAGE_NAME}" \
    bash -lc "${CONTAINER_CMD}"

echo "Container build completed. Artifacts are in ./build"
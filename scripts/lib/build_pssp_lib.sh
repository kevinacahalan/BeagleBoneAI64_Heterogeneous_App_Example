#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
# shellcheck source=pssp_config.sh
source "${SCRIPT_DIR}/pssp_config.sh"

FORCE="false"
SHOW_HELP="false"

print_help() {
    cat <<EOF
Usage: ./scripts/lib/build_pssp_lib.sh [options]

Build PSSP rpmsg_lib.lib with clpru (runs inside the TI container).

Prefer: ./scripts/build.sh --setup (or --build-pssp).

Options:
    --force     Rebuild even if lib/rpmsg_lib.lib already exists
    --repo-root <path>   Repository root (default: auto-detected)
    -h, --help  Show this help

Requires:
    PRU_CGT pointing at TI PRU CGT (Dockerfile.ti sets /usr/share/ti/cgt-pru)
    PSSP already fetched (./scripts/lib/fetch_pssp.sh)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --force)
            FORCE="true"
            shift
            ;;
        --repo-root)
            if [[ $# -lt 2 ]]; then
                echo "Error: --repo-root requires a value" >&2
                exit 1
            fi
            REPO_ROOT="$2"
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

PSSP_DIR="$(pssp_resolve_dir "${REPO_ROOT}")"
SRC_DIR="${PSSP_DIR}/lib/src/rpmsg_lib"
OUT_LIB="${PSSP_DIR}/lib/rpmsg_lib.lib"
BUILT_LIB="${SRC_DIR}/gen/rpmsg_lib.lib"

if [[ ! -d "${PSSP_DIR}" ]] || ! pssp_headers_ok "${PSSP_DIR}"; then
    echo "Error: PSSP not found or incomplete at ${PSSP_DIR}" >&2
    echo "Run: ./scripts/lib/fetch_pssp.sh" >&2
    exit 1
fi

if [[ "${FORCE}" != "true" ]] && pssp_lib_ok "${PSSP_DIR}"; then
    echo "PSSP rpmsg_lib already built: ${OUT_LIB}"
    exit 0
fi

if [[ -z "${PRU_CGT:-}" ]]; then
    if [[ -x /usr/share/ti/cgt-pru/bin/clpru ]]; then
        export PRU_CGT=/usr/share/ti/cgt-pru
    else
        echo "Error: PRU_CGT is not set and /usr/share/ti/cgt-pru was not found" >&2
        echo "Build via ./scripts/build.sh --setup (TI container)." >&2
        exit 1
    fi
fi

if [[ ! -x "${PRU_CGT}/bin/clpru" || ! -x "${PRU_CGT}/bin/arpru" ]]; then
    echo "Error: clpru/arpru not found under ${PRU_CGT}/bin" >&2
    exit 1
fi

if [[ ! -d "${SRC_DIR}" ]]; then
    echo "Error: missing ${SRC_DIR}" >&2
    exit 1
fi

echo "Building PSSP rpmsg_lib (PRU_CGT=${PRU_CGT}) ..."
make -C "${SRC_DIR}" clean || true
make -C "${SRC_DIR}"

if [[ ! -f "${BUILT_LIB}" ]]; then
    echo "Error: expected build output missing: ${BUILT_LIB}" >&2
    exit 1
fi

mkdir -p "$(dirname "${OUT_LIB}")"
cp -f "${BUILT_LIB}" "${OUT_LIB}"
echo "Installed ${OUT_LIB}"

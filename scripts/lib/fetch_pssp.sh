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
Usage: ./scripts/lib/fetch_pssp.sh [options]

Clone the TI PRU Software Support Package into ${PSSP_DIR_REL} at a pinned commit.

Prefer: ./scripts/build.sh --setup (or --fetch-pssp).

Options:
    --force     Remove existing tree and re-clone
    --repo-root <path>   Repository root (default: auto-detected)
    -h, --help  Show this help

Pinned:
    ${PSSP_REPO_URL}
    commit ${PSSP_COMMIT}
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
PARENT="$(dirname "${PSSP_DIR}")"

mkdir -p "${PARENT}"

if [[ ! -w "${PARENT}" ]]; then
    echo "Error: not writable: ${PARENT}" >&2
    exit 1
fi

if [[ "${FORCE}" != "true" ]] && pssp_commit_ok "${PSSP_DIR}" && pssp_headers_ok "${PSSP_DIR}"; then
    echo "PSSP already present at ${PSSP_DIR} (commit ${PSSP_COMMIT})"
    exit 0
fi

if [[ -e "${PSSP_DIR}" ]]; then
    if [[ ! -w "${PSSP_DIR}" ]] || [[ ! -w "$(dirname "${PSSP_DIR}")" ]]; then
        echo "Error: cannot replace ${PSSP_DIR} (permission denied)." >&2
        echo "A prior container run likely created it as root/nobody." >&2
        echo "Fix once: sudo chown -R \"\$(id -u):\$(id -g)\" \"${PSSP_DIR}\"" >&2
        echo "Then re-run, or: sudo rm -rf \"${PSSP_DIR}\"" >&2
        exit 1
    fi
    echo "Removing existing PSSP tree at ${PSSP_DIR} ..."
    rm -rf "${PSSP_DIR}"
fi

if ! command -v git >/dev/null 2>&1; then
    echo "Error: git is required to fetch PSSP" >&2
    exit 1
fi

echo "Cloning PSSP (${PSSP_COMMIT}) from ${PSSP_REPO_URL} ..."
mkdir -p "${PSSP_DIR}"
git -C "${PSSP_DIR}" init -q
git -C "${PSSP_DIR}" remote add origin "${PSSP_REPO_URL}"
# Shallow fetch of the pinned commit (faster than a full clone).
if ! git -C "${PSSP_DIR}" fetch --depth 1 origin "${PSSP_COMMIT}"; then
    echo "Error: git fetch of ${PSSP_COMMIT} failed" >&2
    rm -rf "${PSSP_DIR}"
    exit 1
fi
git -C "${PSSP_DIR}" checkout --detach FETCH_HEAD

if ! pssp_headers_ok "${PSSP_DIR}"; then
    echo "Error: PSSP checkout is missing required headers under ${PSSP_DIR}/include" >&2
    exit 1
fi

printf '%s\n' "${PSSP_COMMIT}" > "${PSSP_DIR}/.pssp_commit"
echo "PSSP ready at ${PSSP_DIR}"

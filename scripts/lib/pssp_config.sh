# Shared config for the TI PRU Software Support Package (PSSP).
# Used by fetch_pssp.sh / build_pssp_lib.sh and sourced from build.sh.

# Prefer the community mirror (same tree as git.ti.com; HTTPS-friendly).
PSSP_REPO_URL="${PSSP_REPO_URL:-https://github.com/dinuxbg/pru-software-support-package.git}"
# Upstream: https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/
PSSP_COMMIT="${PSSP_COMMIT:-f7f23b449532bbe6c464347e4d2e26df374e0a9a}"

# Relative to the repository root (workspace mount in the TI container).
PSSP_DIR_REL="third_party/pru-software-support-package"

pssp_resolve_dir() {
    local repo_root="${1:-}"
    if [[ -z "${repo_root}" ]]; then
        echo "Error: pssp_resolve_dir requires repo root" >&2
        return 1
    fi
    echo "${repo_root}/${PSSP_DIR_REL}"
}

pssp_headers_ok() {
    local root="$1"
    [[ -f "${root}/include/rsc_types.h" \
        && -f "${root}/include/pru_rpmsg.h" \
        && -d "${root}/include/j721e" ]]
}

pssp_lib_ok() {
    local root="$1"
    [[ -f "${root}/lib/rpmsg_lib.lib" && -s "${root}/lib/rpmsg_lib.lib" ]]
}

pssp_commit_ok() {
    local root="$1"
    local marker="${root}/.pssp_commit"
    [[ -f "${marker}" && "$(cat "${marker}")" == "${PSSP_COMMIT}" ]]
}

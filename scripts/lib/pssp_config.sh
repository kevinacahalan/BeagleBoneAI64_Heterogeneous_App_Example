# Shared config for the TI PRU Software Support Package (PSSP).
# Used by fetch_pssp.sh / build_pssp_lib.sh and sourced from build.sh.
#
# Lives next to the Processor SDK under the TI install dir (default ~/ti),
# same pattern as the SDK/PDK — not inside the git workspace.

# Prefer the community mirror (same tree as git.ti.com; HTTPS-friendly).
PSSP_REPO_URL="${PSSP_REPO_URL:-https://github.com/dinuxbg/pru-software-support-package.git}"
# Upstream: https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/
PSSP_COMMIT="${PSSP_COMMIT:-f7f23b449532bbe6c464347e4d2e26df374e0a9a}"

PSSP_DIR_NAME="pru-software-support-package"

# Resolve PSSP path under a TI install directory.
# Host default: $HOME/ti/pru-software-support-package
# Container:    /home/builder/ti/pru-software-support-package
pssp_resolve_dir() {
    local ti_dir="${1:-}"
    if [[ -z "${ti_dir}" ]]; then
        ti_dir="${TI_SDK_DIR:-${HOME}/ti}"
    fi
    echo "${ti_dir}/${PSSP_DIR_NAME}"
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

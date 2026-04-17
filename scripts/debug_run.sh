#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ORIGINAL_USER="${SUDO_USER:-$USER}"
COMPILE=false
SESSION_NAME="LINUX_AND_R5"
LINUX_BINARY="$SCRIPT_DIR/../build/linux/LINUX_SIDE_aarch64"
R5_BINARY="$SCRIPT_DIR/../build/R5_0/r5f_r5f0_0.elf"

print_info() {
    echo "[info] $1"
}

print_warning() {
    echo "[warn] $1"
}

print_error() {
    echo "[error] $1" >&2
}

print_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    --compile      Build the BeagleBone outputs before launching tmux
    --help         Show this help text
EOF
}

validate_device_model() {
    local device_model

    if [ ! -r /proc/device-tree/model ]; then
        print_error "Unable to read /proc/device-tree/model"
        return 1
    fi

    device_model="$(tr -d '\000' < /proc/device-tree/model)"
    if [ "$device_model" != "BeagleBoard.org BeagleBone AI-64" ]; then
        print_error "This script should only be run on a BeagleBone AI-64"
        print_error "Detected model: $device_model"
        return 1
    fi

    return 0
}

validate_requirements() {
    if ! command -v tmux >/dev/null 2>&1; then
        print_error "tmux is required to run this script"
        return 1
    fi

    if [ ! -x "$SCRIPT_DIR/get_remoteproc_number.sh" ]; then
        print_error "Missing helper script: $SCRIPT_DIR/get_remoteproc_number.sh"
        return 1
    fi

    return 0
}

validate_artifacts() {
    if [ ! -f "$R5_BINARY" ]; then
        print_error "Missing R5 firmware: $R5_BINARY"
        return 1
    fi

    if [ ! -f "$LINUX_BINARY" ]; then
        print_error "Missing Linux binary: $LINUX_BINARY"
        return 1
    fi

    return 0
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --compile)
            COMPILE=true
            shift
            ;;
        --help|-h|help)
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

validate_device_model || exit 1
validate_requirements || exit 1

print_info "Using kernel 6 workflow"

if [ "$COMPILE" = true ]; then
    print_info "Building BeagleBone outputs before launch"
    sudo -u "$ORIGINAL_USER" bash "$SCRIPT_DIR/build_script.sh" --beaglebone
fi

validate_artifacts || exit 1

J7_MAIN_R5F0_0_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-main-r5f0_0)"

print_info "Applying EPWM workaround (get around DAbt handler crash for if R5 touches PWM first)"
if ! "$SCRIPT_DIR/enable_epwm4_from_linux.sh"; then
    print_error "EPWM workaround failed; verify the overlay exported P9_25 and that /dev/beagle/pwm/P9_25 is usable"
    exit 1
fi

if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
    print_info "Killing existing tmux session $SESSION_NAME"
    tmux kill-session -t "$SESSION_NAME"
fi

print_info "Starting tmux session $SESSION_NAME"
tmux new-session -d -s "$SESSION_NAME" -n main
tmux setw -t "$SESSION_NAME:main" remain-on-exit on

tmux send-keys -t "$SESSION_NAME:main.0" "sudo \"$SCRIPT_DIR/start_firmware_over_remoteproc.sh\" \"$J7_MAIN_R5F0_0_rproc_number\" \"$R5_BINARY\" || echo 'Failed to start R5 firmware'" C-m
tmux split-window -v -t "$SESSION_NAME:main"
tmux send-keys -t "$SESSION_NAME:main.1" "sudo \"$SCRIPT_DIR/print_trace0.py\" \"$J7_MAIN_R5F0_0_rproc_number\"" C-m
tmux split-window -h -t "$SESSION_NAME:main.1"
tmux send-keys -t "$SESSION_NAME:main.2" "sudo \"$LINUX_BINARY\" || echo 'Failed to start Linux code'" C-m

tmux select-pane -t "$SESSION_NAME:main.0"
tmux attach -t "$SESSION_NAME"
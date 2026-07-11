#!/bin/bash
# Combined Linux + R5 debug session (tmux LINUX_AND_R5).
#
#   sudo ./scripts/debug_run.sh              # create/attach combined session
#   sudo ./scripts/debug_run.sh --compile    # build both sides, then launch
#   sudo ./scripts/debug_run.sh restart-r5   # restart R5 only (Linux stays up)
#   sudo ./scripts/debug_run.sh restart-linux
#   sudo ./scripts/debug_run.sh status
#   sudo ./scripts/debug_run.sh attach
#   sudo ./scripts/debug_run.sh stop-session # tear down tmux only (R5 left as-is)
#
# Independent control (also fine while LINUX_AND_R5 is attached):
#   sudo ./scripts/debug_r5.sh {start|stop|restart|status}
#   sudo ./scripts/debug_linux.sh {start|stop|restart|status}

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ORIGINAL_USER="${SUDO_USER:-$USER}"
COMPILE=false
SESSION_NAME="LINUX_AND_R5"
STANDALONE_LINUX_SESSION_NAME="EXAMPLE_LINUX"
LINUX_BINARY="$SCRIPT_DIR/../build/linux/LINUX_SIDE_aarch64"
R5_BINARY="$SCRIPT_DIR/../build/R5_0/r5f_r5f0_0.elf"
LINUX_PANE="${SESSION_NAME}:main.2"
R5_PANE="${SESSION_NAME}:main.0"
TRACE_PANE="${SESSION_NAME}:main.1"
ACTION="launch"

print_info() {
    echo "[info] $1"
}

print_error() {
    echo "[error] $1" >&2
}

print_help() {
    cat << EOF
Usage: $0 [OPTIONS] [COMMAND]

Commands:
    (default)       Create the LINUX_AND_R5 tmux session and attach
    restart-r5      Restart R5 firmware only (Linux process/session untouched)
    restart-linux   Restart Linux app only (R5 untouched)
    status          Show R5 remoteproc + Linux process + tmux state
    attach          Attach to LINUX_AND_R5 if it exists
    stop-session    Kill the LINUX_AND_R5 tmux session only (does not stop R5)

Options:
    --compile       Build both sides before launching (launch only)
    --help          Show this help text

Pane layout: 0 = R5 start/trace dump, 1 = live trace0, 2 = Linux app
EOF
}

ensure_standalone_linux_session_not_running() {
    if tmux has-session -t "$STANDALONE_LINUX_SESSION_NAME" 2>/dev/null; then
        print_error "tmux session $STANDALONE_LINUX_SESSION_NAME is already running"
        print_error "Stop it before starting $SESSION_NAME"
        return 1
    fi
    return 0
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

session_active() {
    tmux has-session -t "$SESSION_NAME" 2>/dev/null
}

do_status() {
    "$SCRIPT_DIR/debug_r5.sh" status
    echo ""
    "$SCRIPT_DIR/debug_linux.sh" status
}

do_attach() {
    if session_active; then
        tmux attach -t "$SESSION_NAME"
    else
        print_error "No session $SESSION_NAME. Launch with: $0"
        exit 1
    fi
}

do_stop_session() {
    if session_active; then
        print_info "Killing tmux session $SESSION_NAME (R5 remoteproc left as-is)"
        tmux kill-session -t "$SESSION_NAME"
    else
        print_info "No session $SESSION_NAME"
    fi
    if pgrep -f "LINUX_SIDE_aarch64" >/dev/null 2>&1; then
        print_info "Stopping leftover LINUX_SIDE_aarch64"
        sudo pkill -f "LINUX_SIDE_aarch64" || true
    fi
}

do_restart_r5() {
    print_info "Restarting R5 only (Linux left running)"
    "$SCRIPT_DIR/debug_r5.sh" restart

    if session_active; then
        J7_MAIN_R5F0_0_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-main-r5f0_0)"
        # Refresh live trace so it follows the new firmware image.
        tmux send-keys -t "$TRACE_PANE" C-c 2>/dev/null || true
        sleep 0.2
        tmux send-keys -t "$TRACE_PANE" "sudo \"$SCRIPT_DIR/print_trace0.py\" \"$J7_MAIN_R5F0_0_rproc_number\"" C-m
    fi
}

do_restart_linux() {
    print_info "Restarting Linux only"
    "$SCRIPT_DIR/debug_linux.sh" restart
}

do_launch() {
    ensure_standalone_linux_session_not_running || exit 1
    validate_artifacts || exit 1

    J7_MAIN_R5F0_0_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-main-r5f0_0)"

    print_info "Applying EPWM workaround (get around DAbt handler crash for if R5 touches PWM first)"
    if ! "$SCRIPT_DIR/enable_epwm4_from_linux.sh"; then
        print_error "EPWM workaround failed; verify the overlay exported P9_25 and that /dev/beagle/pwm/P9_25 is usable"
        exit 1
    fi

    if session_active; then
        print_info "Killing existing tmux session $SESSION_NAME"
        tmux kill-session -t "$SESSION_NAME"
    fi

    print_info "Starting tmux session $SESSION_NAME"
    tmux new-session -d -s "$SESSION_NAME" -n main
    tmux setw -t "$SESSION_NAME:main" remain-on-exit on

    tmux send-keys -t "$R5_PANE" "sudo \"$SCRIPT_DIR/start_firmware_over_remoteproc.sh\" \"$J7_MAIN_R5F0_0_rproc_number\" \"$R5_BINARY\" || echo 'Failed to start R5 firmware'" C-m
    tmux split-window -v -t "$SESSION_NAME:main"
    tmux send-keys -t "$TRACE_PANE" "sudo \"$SCRIPT_DIR/print_trace0.py\" \"$J7_MAIN_R5F0_0_rproc_number\"" C-m
    tmux split-window -h -t "$TRACE_PANE"
    tmux send-keys -t "$LINUX_PANE" "sudo \"$LINUX_BINARY\" || echo 'Failed to start Linux code'" C-m

    tmux select-pane -t "$R5_PANE"
    tmux attach -t "$SESSION_NAME"
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
        restart-r5|restart-linux|status|attach|stop-session)
            ACTION="$1"
            shift
            ;;
        *)
            print_error "Unknown option/command: $1"
            print_help
            exit 1
            ;;
    esac
done

validate_device_model || exit 1
validate_requirements || exit 1

if [ "$COMPILE" = true ]; then
    if [ "$ACTION" != "launch" ]; then
        print_error "--compile is only valid with the default launch"
        exit 1
    fi
    print_info "Building BeagleBone outputs before launch"
    sudo -u "$ORIGINAL_USER" bash "$SCRIPT_DIR/build.sh" --both
fi

print_info "Using kernel 6 workflow"

case "$ACTION" in
    launch)
        do_launch
        ;;
    restart-r5)
        do_restart_r5
        ;;
    restart-linux)
        do_restart_linux
        ;;
    status)
        do_status
        ;;
    attach)
        do_attach
        ;;
    stop-session)
        do_stop_session
        ;;
esac

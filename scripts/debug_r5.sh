#!/bin/bash
# Start/stop the example app's R5 firmware independently for debugging.
# Usage:
#   sudo ./scripts/debug_r5.sh start
#   sudo ./scripts/debug_r5.sh stop
#   sudo ./scripts/debug_r5.sh restart
#   sudo ./scripts/debug_r5.sh status
#   sudo ./scripts/debug_r5.sh trace
#   sudo ./scripts/debug_r5.sh run

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
R5_ELF="$PROJECT_DIR/build/R5_0/r5f_r5f0_0.elf"

DEVICE_MODEL=$(cat /proc/device-tree/model | sed "s/ /_/g" | tr -d '\000')
if [ "$DEVICE_MODEL" != "BeagleBoard.org_BeagleBone_AI-64" ]; then
    echo "Error: This script should only be run on a BeagleBone AI-64."
    exit 1
fi

J7_MAIN_R5F0_0_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-main-r5f0_0)"

print_help() {
    echo "Usage: $0 {start|stop|restart|status|trace|run}"
    echo ""
    echo "Commands:"
    echo "  start    Start the example R5 firmware"
    echo "  stop     Stop the example R5 firmware"
    echo "  restart  Stop then start the example R5 firmware"
    echo "  status   Show the current remoteproc state"
    echo "  trace    Show R5 trace0 output (Ctrl+C to exit)"
    echo "  run      Restart if running, otherwise start; then trace"
    echo ""
    exit 0
}

get_r5_state() {
    cat /sys/class/remoteproc/remoteproc${J7_MAIN_R5F0_0_rproc_number}/state 2>/dev/null || echo "unknown"
}

do_stop() {
    local state
    state=$(get_r5_state)
    if [ "$state" = "offline" ]; then
        echo "R5 firmware already stopped."
        return
    fi

    echo "Stopping R5 firmware (remoteproc${J7_MAIN_R5F0_0_rproc_number})..."
    echo stop | sudo tee /sys/class/remoteproc/remoteproc${J7_MAIN_R5F0_0_rproc_number}/state >/dev/null 2>&1 || true
    echo "R5 firmware stopped."
}

do_start() {
    local state

    if [ ! -f "$R5_ELF" ]; then
        echo "Error: R5 ELF not found at $R5_ELF"
        echo "Build first with: ./scripts/build_script.sh --r5"
        exit 1
    fi

    "$SCRIPT_DIR/enable_epwm4_from_linux.sh"

    state=$(get_r5_state)
    if [ "$state" = "running" ]; then
        echo "R5 firmware already running."
        return
    fi

    echo "Copying R5 ELF to /lib/firmware/..."
    sudo cp -f "$R5_ELF" /lib/firmware/

    echo "Setting firmware name..."
    echo "$(basename "$R5_ELF")" | sudo tee /sys/class/remoteproc/remoteproc${J7_MAIN_R5F0_0_rproc_number}/firmware >/dev/null

    echo "Starting R5 firmware (remoteproc${J7_MAIN_R5F0_0_rproc_number})..."
    echo start | sudo tee /sys/class/remoteproc/remoteproc${J7_MAIN_R5F0_0_rproc_number}/state >/dev/null

    sleep 1
    state=$(get_r5_state)
    if [ "$state" != "running" ]; then
        echo "Error: R5 firmware failed to start (state: $state)"
        exit 1
    fi

    echo "R5 firmware started."
}

do_trace() {
    echo "=== R5 trace0 output (Ctrl+C to exit) ==="
    sudo "$SCRIPT_DIR/print_trace0.py" "$J7_MAIN_R5F0_0_rproc_number"
}

do_run() {
    local state
    state=$(get_r5_state)
    if [ "$state" = "running" ]; then
        echo "R5 firmware already running; restarting before trace..."
        do_stop
        do_start
    else
        do_start
    fi
    do_trace
}

do_status() {
    local state
    local fw

    state=$(cat /sys/class/remoteproc/remoteproc${J7_MAIN_R5F0_0_rproc_number}/state 2>/dev/null || echo "unknown")
    fw=$(cat /sys/class/remoteproc/remoteproc${J7_MAIN_R5F0_0_rproc_number}/firmware 2>/dev/null || echo "unknown")
    echo "remoteproc${J7_MAIN_R5F0_0_rproc_number} (j7-main-r5f0_0):"
    echo "  State:    $state"
    echo "  Firmware: $fw"
}

if [ $# -lt 1 ]; then
    print_help
fi

case "$1" in
    start)
        do_start
        ;;
    stop)
        do_stop
        ;;
    restart)
        do_stop
        do_start
        ;;
    status)
        do_status
        ;;
    run)
        do_run
        ;;
    trace)
        do_trace
        ;;
    attach)
        do_trace
        ;;
    help|--help|-h)
        print_help
        ;;
    *)
        echo "Unknown command: $1"
        print_help
        ;;
esac
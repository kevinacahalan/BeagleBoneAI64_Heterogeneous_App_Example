#!/bin/bash
# Start/stop PRU0_0 hello firmware (j7-pru0_0).
#
# Usage (from repo root or this demo):
#   sudo ./extra-examples/pru0_0-hello/scripts/run.sh start
#   sudo ./extra-examples/pru0_0-hello/scripts/run.sh stop
#   sudo ./extra-examples/pru0_0-hello/scripts/run.sh restart
#   sudo ./extra-examples/pru0_0-hello/scripts/run.sh status
#   sudo ./extra-examples/pru0_0-hello/scripts/run.sh trace
#   sudo ./extra-examples/pru0_0-hello/scripts/run.sh run

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEMO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$DEMO_DIR/../.." && pwd)"
ROOT_SCRIPTS="$REPO_ROOT/scripts"
FW="$REPO_ROOT/build/extra-examples/pru0_0-hello/pru0_0-hello.elf"
STOP_TIMEOUT_SEC=10

# Print paths relative to the current working directory.
relpath() {
    local path="$1"
    if realpath -m --relative-to=. "$path" >/dev/null 2>&1; then
        realpath -m --relative-to=. "$path"
    else
        python3 -c 'import os, sys; print(os.path.relpath(sys.argv[1]))' "$path"
    fi
}

DEVICE_MODEL=$(cat /proc/device-tree/model | sed "s/ /_/g" | tr -d '\000')
if [ "$DEVICE_MODEL" != "BeagleBoard.org_BeagleBone_AI-64" ]; then
    echo "Error: This script should only be run on a BeagleBone AI-64."
    exit 1
fi

PRU0_0_rproc_number="$($ROOT_SCRIPTS/get_remoteproc_number.sh j7-pru0_0)"

print_help() {
    echo "Usage: $0 {start|stop|restart|status|trace|run}"
    echo ""
    echo "PRU0_0 hello (remoteproc trace only):"
    echo "  start     Start pru0_0-hello.elf"
    echo "  stop      Stop PRU0_0 firmware"
    echo "  restart   Stop then start"
    echo "  status    Show remoteproc state"
    echo "  trace     Show remoteproc trace0 (Ctrl+C to exit)"
    echo "  run       Restart if running, otherwise start; then trace"
    echo ""
    echo "Firmware: $(relpath "$FW")"
    echo "Build:    ./scripts/build.sh --extra pru0_0-hello"
    exit 0
}

get_pru_state() {
    cat /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/state 2>/dev/null || echo "unknown"
}

wait_for_offline() {
    local state
    local i

    for i in $(seq 1 "$STOP_TIMEOUT_SEC"); do
        state=$(get_pru_state)
        if [ "$state" = "offline" ]; then
            return 0
        fi
        sleep 1
    done

    state=$(get_pru_state)
    echo "Error: PRU0_0 did not reach offline within ${STOP_TIMEOUT_SEC}s (state: $state)."
    return 1
}

do_stop() {
    local state
    state=$(get_pru_state)
    if [ "$state" = "offline" ]; then
        echo "PRU0_0 firmware already stopped."
        return
    fi

    echo "Stopping PRU0_0 (remoteproc${PRU0_0_rproc_number})..."
    if ! echo stop | sudo tee /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/state >/dev/null; then
        echo "Error: echo stop failed (see dmesg)."
        exit 1
    fi

    if ! wait_for_offline; then
        exit 1
    fi

    echo "PRU0_0 firmware stopped (state: offline)."
}

do_start() {
    local state

    if [ ! -f "$FW" ]; then
        echo "Error: firmware not found at $(relpath "$FW")"
        echo "Build first with: ./scripts/build.sh --extra pru0_0-hello"
        exit 1
    fi

    state=$(get_pru_state)
    if [ "$state" = "running" ]; then
        echo "PRU0_0 firmware already running."
        return
    fi

    if [ "$state" != "offline" ]; then
        echo "Error: PRU0_0 remoteproc state is '$state' (expected offline before start)."
        exit 1
    fi

    echo "Copying $(basename "$FW") to /lib/firmware/..."
    sudo cp -f "$FW" /lib/firmware/

    echo "$(basename "$FW")" | sudo tee /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/firmware >/dev/null

    echo "Starting PRU0_0 (remoteproc${PRU0_0_rproc_number})..."
    echo start | sudo tee /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/state >/dev/null

    sleep 1
    state=$(get_pru_state)
    if [ "$state" != "running" ]; then
        echo "Error: PRU0_0 firmware failed to start (state: $state)"
        exit 1
    fi

    echo "PRU0_0 hello firmware started."
}

do_trace() {
    echo "=== PRU0_0 trace0 (Ctrl+C to exit) ==="
    sudo "$ROOT_SCRIPTS/print_trace0.py" "$PRU0_0_rproc_number"
}

do_run() {
    local state
    state=$(get_pru_state)
    if [ "$state" = "running" ]; then
        echo "PRU0_0 already running; restarting before trace..."
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

    state=$(cat /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/state 2>/dev/null || echo "unknown")
    fw=$(cat /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/firmware 2>/dev/null || echo "unknown")
    echo "remoteproc${PRU0_0_rproc_number} (j7-pru0_0 / b034000.pru):"
    echo "  State:    $state"
    echo "  Firmware: $fw"
}

if [ $# -lt 1 ]; then
    print_help
fi

CMD="$1"

case "$CMD" in
    start) do_start ;;
    stop) do_stop ;;
    restart) do_stop; do_start ;;
    status) do_status ;;
    run) do_run ;;
    trace) do_trace ;;
    help|--help|-h) print_help ;;
    *)
        echo "Unknown command: $CMD"
        print_help
        ;;
esac

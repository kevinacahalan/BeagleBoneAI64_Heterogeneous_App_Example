#!/bin/bash
# Start/stop RTU0_0 + PRU0_0 cooperative LED demo.
#
# RTU owns RPMsg (rpmsg-raw port 30); PRU blinks P8_11 from shared DMEM.
# Do not run alongside phase-1 `debug_pru0_0.sh start rpmsg_led` (same port 30).
#
# Usage:
#   sudo ./scripts/debug_rtu_pru_led.sh start
#   sudo ./scripts/debug_rtu_pru_led.sh stop
#   sudo ./scripts/debug_rtu_pru_led.sh restart
#   sudo ./scripts/debug_rtu_pru_led.sh status
#   sudo ./scripts/debug_rtu_pru_led.sh trace
#   sudo ./scripts/debug_rtu_pru_led.sh trace-split
#   sudo ./scripts/debug_rtu_pru_led.sh run

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PRU_FW_DIR="$PROJECT_DIR/build/PRU0_0"
RTU_FW_DIR="$PROJECT_DIR/build/RTU0_0"
PRU_FW="$PRU_FW_DIR/pru0_0-led-worker.out"
RTU_FW="$RTU_FW_DIR/rtu0_0-rpmsg-led.out"
STOP_TIMEOUT_SEC=10

DEVICE_MODEL=$(cat /proc/device-tree/model | sed "s/ /_/g" | tr -d '\000')
if [ "$DEVICE_MODEL" != "BeagleBoard.org_BeagleBone_AI-64" ]; then
    echo "Error: This script should only be run on a BeagleBone AI-64."
    exit 1
fi

RTU_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-rtu0_0)"
PRU_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-pru0_0)"

print_help() {
    echo "Usage: $0 {start|stop|restart|status|trace|trace-split|run}"
    echo ""
    echo "Cooperative LED demo (RTU RPMsg + PRU blink on P8_11):"
    echo "  start        Start RTU first, then PRU led_worker"
    echo "  stop         Stop PRU then RTU"
    echo "  restart      stop + start"
    echo "  status       Show both remoteproc states"
    echo "  trace        Live dual trace0 (merged, labeled) — default"
    echo "  trace-split  Live dual trace0 in a tmux vertical split"
    echo "  run          restart + trace"
    echo ""
    echo "Firmware:"
    echo "  $RTU_FW"
    echo "  $PRU_FW"
    echo ""
    echo "Host: sudo python3 PRU0_0_SIDE/host/blink_count.py <count>"
    echo "Do not run phase-1 rpmsg_led at the same time (both use port 30)."
    exit 0
}

get_state() {
    local num="$1"
    cat /sys/class/remoteproc/remoteproc${num}/state 2>/dev/null || echo "unknown"
}

wait_for_offline() {
    local num="$1"
    local label="$2"
    local state
    local i

    for i in $(seq 1 "$STOP_TIMEOUT_SEC"); do
        state=$(get_state "$num")
        if [ "$state" = "offline" ]; then
            return 0
        fi
        sleep 1
    done

    state=$(get_state "$num")
    echo "Error: $label did not reach offline within ${STOP_TIMEOUT_SEC}s (state: $state)."
    return 1
}

stop_one() {
    local num="$1"
    local label="$2"
    local state

    state=$(get_state "$num")
    if [ "$state" = "offline" ]; then
        echo "$label already stopped."
        return
    fi

    echo "Stopping $label (remoteproc${num})..."
    if ! echo stop | sudo tee /sys/class/remoteproc/remoteproc${num}/state >/dev/null; then
        echo "Error: echo stop failed for $label (see dmesg)."
        exit 1
    fi

    if ! wait_for_offline "$num" "$label"; then
        exit 1
    fi
    echo "$label stopped."
}

do_stop() {
    # Stop PRU first so it is not mid-blink while RTU tears down RPMsg.
    stop_one "$PRU_rproc_number" "PRU0_0"
    stop_one "$RTU_rproc_number" "RTU0_0"
}

start_one() {
    local num="$1"
    local label="$2"
    local fw="$3"
    local state

    if [ ! -f "$fw" ]; then
        echo "Error: firmware not found at $fw"
        echo "Build first with: ./scripts/build.sh --pru"
        exit 1
    fi

    state=$(get_state "$num")
    if [ "$state" = "running" ]; then
        echo "$label already running."
        return
    fi

    if [ "$state" != "offline" ]; then
        echo "Error: $label remoteproc state is '$state' (expected offline before start)."
        exit 1
    fi

    echo "Copying $(basename "$fw") to /lib/firmware/..."
    sudo cp -f "$fw" /lib/firmware/

    echo "$(basename "$fw")" | sudo tee /sys/class/remoteproc/remoteproc${num}/firmware >/dev/null

    echo "Starting $label (remoteproc${num})..."
    echo start | sudo tee /sys/class/remoteproc/remoteproc${num}/state >/dev/null

    sleep 1
    state=$(get_state "$num")
    if [ "$state" != "running" ]; then
        echo "Error: $label failed to start (state: $state). Check dmesg (vring / resource table)."
        exit 1
    fi
    echo "$label started."
}

find_rpmsg_port30() {
    local found=""
    local d dst_raw dst_dec n

    for d in /sys/bus/rpmsg/devices/*; do
        [ -e "$d/dst" ] || continue
        dst_raw=$(cat "$d/dst" 2>/dev/null || true)
        dst_dec=$(printf "%d" "$dst_raw" 2>/dev/null || true)
        if [ "$dst_dec" = "30" ]; then
            for n in "$d"/rpmsg/rpmsg*; do
                [ -e "$n" ] || continue
                found="/dev/$(basename "$n")"
                break
            done
        fi
        [ -n "$found" ] && break
    done
    echo "$found"
}

do_start() {
    # RTU first so rpmsg-raw channel exists before PRU worker runs.
    start_one "$RTU_rproc_number" "RTU0_0" "$RTU_FW"
    start_one "$PRU_rproc_number" "PRU0_0" "$PRU_FW"

    sudo modprobe rpmsg_char 2>/dev/null || true
    sleep 0.5

    found="$(find_rpmsg_port30)"
    if [ -n "$found" ]; then
        echo "RPMsg device ready: $found (port 30)"
    else
        echo "Note: rpmsg-raw port 30 not visible yet; use:"
        echo "  sudo python3 PRU0_0_SIDE/host/blink_count.py <count>"
        echo "If RTU boot failed with 'IRQ vring not found', rebuild/install the overlay and reboot."
    fi
}

do_status() {
    local rtu_state pru_state rtu_fw pru_fw

    rtu_state=$(get_state "$RTU_rproc_number")
    pru_state=$(get_state "$PRU_rproc_number")
    rtu_fw=$(cat /sys/class/remoteproc/remoteproc${RTU_rproc_number}/firmware 2>/dev/null || echo "unknown")
    pru_fw=$(cat /sys/class/remoteproc/remoteproc${PRU_rproc_number}/firmware 2>/dev/null || echo "unknown")

    echo "remoteproc${RTU_rproc_number} (j7-rtu0_0):"
    echo "  State:    $rtu_state"
    echo "  Firmware: $rtu_fw"
    echo "remoteproc${PRU_rproc_number} (j7-pru0_0):"
    echo "  State:    $pru_state"
    echo "  Firmware: $pru_fw"
}

do_trace() {
    echo "=== RTU0_0 + PRU0_0 dual trace0 (Ctrl+C to exit) ==="
    echo "Merged stream with [RTU0_0]/[PRU0_0] labels (better for handshake order)."
    echo "For a tmux split: $0 trace-split"
    sudo "$SCRIPT_DIR/print_dual_trace.py" \
        "$RTU_rproc_number" "RTU0_0" \
        "$PRU_rproc_number" "PRU0_0"
}

do_trace_split() {
    local session="RTU_PRU_TRACE"

    if ! command -v tmux >/dev/null 2>&1; then
        echo "tmux not found; falling back to merged dual trace."
        do_trace
        return
    fi

    if [ ! -t 1 ]; then
        echo "No TTY for tmux attach; falling back to merged dual trace."
        do_trace
        return
    fi

    echo "=== RTU0_0 | PRU0_0 split trace (tmux session $session) ==="
    tmux kill-session -t "$session" 2>/dev/null || true
    tmux new-session -d -s "$session" -n trace \
        "echo '=== RTU0_0 (remoteproc${RTU_rproc_number}) ==='; sudo '$SCRIPT_DIR/print_trace0.py' '$RTU_rproc_number'"
    tmux split-window -h -t "$session:trace" \
        "echo '=== PRU0_0 (remoteproc${PRU_rproc_number}) ==='; sudo '$SCRIPT_DIR/print_trace0.py' '$PRU_rproc_number'"
    tmux select-layout -t "$session:trace" even-horizontal
    tmux attach-session -t "$session"
}

do_run() {
    local rtu_state pru_state

    rtu_state=$(get_state "$RTU_rproc_number")
    pru_state=$(get_state "$PRU_rproc_number")
    if [ "$rtu_state" = "running" ] || [ "$pru_state" = "running" ]; then
        echo "Firmware already running; restarting before trace..."
        do_stop
    fi
    do_start
    do_trace
}

if [ $# -lt 1 ]; then
    print_help
fi

CMD="$1"

case "$CMD" in
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
    trace)
        do_trace
        ;;
    trace-split)
        do_trace_split
        ;;
    run)
        do_run
        ;;
    help|--help|-h)
        print_help
        ;;
    *)
        echo "Unknown command: $CMD"
        print_help
        ;;
esac

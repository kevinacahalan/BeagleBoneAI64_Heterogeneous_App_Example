#!/bin/bash
# Start/stop PRU0_0 (j7-pru0_0) firmware independently.
#
# Usage:
#   sudo ./scripts/debug_pru0_0.sh start [hello|rpmsg_led]
#   sudo ./scripts/debug_pru0_0.sh stop
#   sudo ./scripts/debug_pru0_0.sh restart [hello|rpmsg_led]
#   sudo ./scripts/debug_pru0_0.sh status
#   sudo ./scripts/debug_pru0_0.sh trace
#   sudo ./scripts/debug_pru0_0.sh run [hello|rpmsg_led]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
FW_DIR="$PROJECT_DIR/build/PRU0_0"
DEFAULT_APP="hello"
STOP_TIMEOUT_SEC=10

DEVICE_MODEL=$(cat /proc/device-tree/model | sed "s/ /_/g" | tr -d '\000')
if [ "$DEVICE_MODEL" != "BeagleBoard.org_BeagleBone_AI-64" ]; then
    echo "Error: This script should only be run on a BeagleBone AI-64."
    exit 1
fi

PRU0_0_rproc_number="$($SCRIPT_DIR/get_remoteproc_number.sh j7-pru0_0)"

resolve_fw() {
    local app="${1:-$DEFAULT_APP}"
    case "$app" in
        hello)
            echo "$FW_DIR/pru0_0-hello.out"
            ;;
        rpmsg_led|rpmsg-led|led)
            echo "$FW_DIR/pru0_0-rpmsg-led.out"
            ;;
        *)
            echo "Unknown firmware app: $app (use hello or rpmsg_led)" >&2
            exit 1
            ;;
    esac
}

print_help() {
    echo "Usage: $0 {start|stop|restart|status|trace|run} [hello|rpmsg_led]"
    echo ""
    echo "Commands (PRU0_0 / remoteproc only):"
    echo "  start [app]    Start firmware (default app: hello)"
    echo "  stop           Stop PRU0_0 firmware"
    echo "  restart [app]  Stop then start"
    echo "  status         Show remoteproc state"
    echo "  trace          Show remoteproc trace0 (Ctrl+C to exit)"
    echo "  run [app]      Restart if running, otherwise start; then trace"
    echo ""
    echo "Apps:"
    echo "  hello       Trace hello-world (pru0_0-hello.out)"
    echo "  rpmsg_led   RPMsg blink-count on P8_11 (pru0_0-rpmsg-led.out)"
    echo ""
    echo "Resolve remoteproc by name j7-pru0_0 (do not hardcode remoteprocN)."
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
    local app="${1:-$DEFAULT_APP}"
    local fw
    local state

    fw="$(resolve_fw "$app")"
    if [ ! -f "$fw" ]; then
        echo "Error: firmware not found at $fw"
        echo "Build first with: ./scripts/build.sh --pru"
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

    echo "Copying $(basename "$fw") to /lib/firmware/..."
    sudo cp -f "$fw" /lib/firmware/

    echo "$(basename "$fw")" | sudo tee /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/firmware >/dev/null

    echo "Starting PRU0_0 (remoteproc${PRU0_0_rproc_number}, app=$app)..."
    echo start | sudo tee /sys/class/remoteproc/remoteproc${PRU0_0_rproc_number}/state >/dev/null

    sleep 1
    state=$(get_pru_state)
    if [ "$state" != "running" ]; then
        echo "Error: PRU0_0 firmware failed to start (state: $state)"
        exit 1
    fi

    if [ "$app" = "rpmsg_led" ] || [ "$app" = "rpmsg-led" ] || [ "$app" = "led" ]; then
        # Kernel 6.12: rpmsg_pru is gone; firmware uses "rpmsg-raw" → rpmsg_char.
        sudo modprobe rpmsg_char 2>/dev/null || true
        sleep 0.5
        found=""
        for d in /sys/bus/rpmsg/devices/*; do
            [ -e "$d/dst" ] || continue
            dst_raw=$(cat "$d/dst" 2>/dev/null || true)
            # Sysfs may be decimal or hex (0x1e)
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
        if [ -n "$found" ]; then
            echo "RPMsg device ready: $found (port 30)"
        else
            echo "Note: rpmsg-raw port 30 not visible yet; use:"
            echo "  python3 PRU0_0_SIDE/host/blink_count.py <count>"
            echo "If boot failed with 'IRQ vring not found', rebuild/install the overlay and reboot."
        fi
    fi

    echo "PRU0_0 firmware started."
}

do_trace() {
    echo "=== PRU0_0 trace0 (Ctrl+C to exit) ==="
    sudo "$SCRIPT_DIR/print_trace0.py" "$PRU0_0_rproc_number"
}

do_run() {
    local app="${1:-$DEFAULT_APP}"
    local state
    state=$(get_pru_state)
    if [ "$state" = "running" ]; then
        echo "PRU0_0 already running; restarting before trace..."
        do_stop
        do_start "$app"
    else
        do_start "$app"
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
shift || true
APP="${1:-$DEFAULT_APP}"

case "$CMD" in
    start)
        do_start "$APP"
        ;;
    stop)
        do_stop
        ;;
    restart)
        do_stop
        do_start "$APP"
        ;;
    status)
        do_status
        ;;
    run)
        do_run "$APP"
        ;;
    trace)
        do_trace
        ;;
    help|--help|-h)
        print_help
        ;;
    *)
        echo "Unknown command: $CMD"
        print_help
        ;;
esac

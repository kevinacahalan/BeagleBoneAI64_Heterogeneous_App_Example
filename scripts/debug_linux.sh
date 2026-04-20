#!/bin/bash
# Start/stop the example app's Linux-side binary independently for debugging.
# Usage:
#   sudo ./scripts/debug_linux.sh start
#   sudo ./scripts/debug_linux.sh stop
#   sudo ./scripts/debug_linux.sh restart
#   sudo ./scripts/debug_linux.sh status
#   sudo ./scripts/debug_linux.sh attach
#   sudo ./scripts/debug_linux.sh run

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LINUX_BIN="$PROJECT_DIR/build/linux/LINUX_SIDE_aarch64"
SESSION_NAME="EXAMPLE_LINUX"
COMBINED_SESSION_NAME="LINUX_AND_R5"

DEVICE_MODEL=$(cat /proc/device-tree/model | sed "s/ /_/g" | tr -d '\000')
if [ "$DEVICE_MODEL" != "BeagleBoard.org_BeagleBone_AI-64" ]; then
    echo "Error: This script should only be run on a BeagleBone AI-64."
    exit 1
fi

print_help() {
    echo "Usage: $0 {start|stop|restart|status|attach|run}"
    echo ""
    echo "Commands:"
    echo "  start    Start the example Linux binary in a tmux session"
    echo "  stop     Stop the example Linux binary"
    echo "  restart  Stop then start the example Linux binary"
    echo "  status   Show whether the example Linux binary is running"
    echo "  attach   Attach to the running tmux session (Ctrl+B, D to detach)"
    echo "  run      Restart if running, otherwise start; then attach"
    echo ""
    exit 0
}

ensure_combined_session_not_running() {
    if tmux has-session -t "$COMBINED_SESSION_NAME" 2>/dev/null; then
        echo "Error: tmux session '$COMBINED_SESSION_NAME' is already running."
        echo "Stop it before starting '$SESSION_NAME' so the standalone Linux session does not conflict with the combined debug session."
        exit 1
    fi
}

do_stop() {
    echo "Stopping example Linux application..."

    if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
        echo "  Killing tmux session: $SESSION_NAME"
        tmux kill-session -t "$SESSION_NAME"
    fi

    if pgrep -f "LINUX_SIDE_aarch64" >/dev/null 2>&1; then
        echo "  Killing stray LINUX_SIDE_aarch64 processes..."
        sudo pkill -f "LINUX_SIDE_aarch64" || true
    fi

    echo "Example Linux application stopped."
}

do_start() {
    if [ ! -f "$LINUX_BIN" ]; then
        echo "Error: Linux binary not found at $LINUX_BIN"
        echo "Build first with: ./scripts/build_script.sh --linux"
        exit 1
    fi

    ensure_combined_session_not_running

    if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
        echo "Example Linux app is already running in tmux session '$SESSION_NAME'."
        echo "Use '$0 stop' first, or '$0 attach' to connect."
        return
    fi

    echo "Starting example Linux application in tmux session '$SESSION_NAME'..."
    tmux new-session -d -s "$SESSION_NAME" -n linux
    tmux setw -t "$SESSION_NAME:linux" remain-on-exit on
    tmux send-keys -t "$SESSION_NAME:linux.0" "sudo \"$LINUX_BIN\"" C-m

    echo "Example Linux application started."
    echo "  Attach with: sudo ./scripts/debug_linux.sh attach"
    echo "  Or directly: tmux attach -t $SESSION_NAME"
}

do_status() {
    echo "=== Example Linux Status ==="

    if tmux has-session -t "$COMBINED_SESSION_NAME" 2>/dev/null; then
        echo "  Combined tmux session ($COMBINED_SESSION_NAME): ACTIVE"
    else
        echo "  Combined tmux session ($COMBINED_SESSION_NAME): none"
    fi

    if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
        echo "  Tmux session ($SESSION_NAME): ACTIVE"
    else
        echo "  Tmux session ($SESSION_NAME): none"
    fi

    local pid
    pid=$(pgrep -f "LINUX_SIDE_aarch64" | head -1 || true)
    if [ -n "$pid" ]; then
        echo "  Process: RUNNING (PID $pid)"
    else
        echo "  Process: not running"
    fi
}

do_attach() {
    if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
        tmux attach -t "$SESSION_NAME"
    else
        echo "No tmux session '$SESSION_NAME' found. Start first with: $0 start"
        exit 1
    fi
}

do_run() {
    if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
        echo "Example Linux app already running; restarting before attach..."
        do_stop
        do_start
    else
        do_start
    fi
    do_attach
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
    attach)
        do_attach
        ;;
    run)
        do_run
        ;;
    help|--help|-h)
        print_help
        ;;
    *)
        echo "Unknown command: $1"
        print_help
        ;;
esac
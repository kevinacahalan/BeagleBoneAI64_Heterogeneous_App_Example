#!/bin/bash
# Stop (if needed) then start R5 firmware via remoteproc only.
# Does not touch Linux processes or tmux sessions — Linux reconnects on its own.
# Live trace belongs in print_trace0.py / debug_r5.sh trace, not this script.

if [ $# -ne 2 ]; then
    echo "Usage: $0 <remoteproc_number> <firmware_name>"
    exit 1
fi

remoteproc_number=$1
firmware_name=$2
STOP_TIMEOUT_SEC=15

wait_for_offline() {
    local state
    local i
    for i in $(seq 1 "$STOP_TIMEOUT_SEC"); do
        state=$(cat /sys/class/remoteproc/remoteproc${remoteproc_number}/state 2>/dev/null || echo "unknown")
        if [ "$state" = "offline" ]; then
            return 0
        fi
        sleep 1
    done
    echo "Error: remoteproc${remoteproc_number} did not reach offline (state: $state)"
    return 1
}

state=$(cat /sys/class/remoteproc/remoteproc${remoteproc_number}/state 2>/dev/null || echo "unknown")
if [ "$state" != "offline" ]; then
    echo "Stopping remoteproc${remoteproc_number}..."
    if ! echo stop | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/state >/dev/null; then
        true
    fi
    if ! wait_for_offline; then
        exit 1
    fi
fi

sudo cp -f "$firmware_name" /lib/firmware/
echo "$(basename "$firmware_name")" | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/firmware
echo start | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/state

sleep 1
state=$(cat /sys/class/remoteproc/remoteproc${remoteproc_number}/state 2>/dev/null || echo "unknown")
if [ "$state" != "running" ]; then
    echo "Error: remoteproc${remoteproc_number} failed to start (state: $state)"
    exit 1
fi

echo "R5 firmware started (remoteproc${remoteproc_number}, state: running)."
echo "Use ./scripts/debug_r5.sh trace  (or the debug_run trace pane) for live trace0."

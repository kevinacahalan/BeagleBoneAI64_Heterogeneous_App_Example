#!/bin/bash
# Stop R5 firmware via remoteproc only. Does not touch Linux or tmux.

if [ $# -ne 1 ]; then
    echo "Usage: $0 <remoteproc_number>"
    exit 1
fi

remoteproc_number=$1
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
if [ "$state" = "offline" ]; then
    echo "R5 firmware already stopped."
    exit 0
fi

echo "Stopping remoteproc${remoteproc_number}..."
if ! echo stop | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/state >/dev/null; then
    echo "Error: echo stop failed (see dmesg for k3_r5_rproc_stop / -16)."
    exit 1
fi

if ! wait_for_offline; then
    exit 1
fi

echo "R5 firmware stopped (state: offline)."

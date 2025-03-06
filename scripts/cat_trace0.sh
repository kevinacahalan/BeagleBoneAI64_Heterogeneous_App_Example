#!/bin/bash

# Check if the argument is supplied
if [ $# -ne 1 ]; then
    echo "Usage: $0 <remoteproc_number>"
    exit 1
fi

remoteproc_id="$1"

sudo cat "/sys/kernel/debug/remoteproc/remoteproc$remoteproc_id/trace0"

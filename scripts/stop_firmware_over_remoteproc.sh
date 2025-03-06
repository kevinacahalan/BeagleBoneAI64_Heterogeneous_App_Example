#!/bin/bash

# Check if the user provided remoteproc_number
if [ $# -ne 1 ]; then
    echo "Usage: $0 <remoteproc_number>"
    exit 1
fi

# Assign arguments to variables for better readability
remoteproc_number=$1

# Stop the current firmware
echo stop | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/state >/dev/null 2>&1

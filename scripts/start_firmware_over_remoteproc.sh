#!/bin/bash

# Check if the user provided two arguments
if [ $# -ne 2 ]; then
    echo "Usage: $0 <remoteproc_number> <firmware_name>"
    exit 1
fi

# Assign arguments to variables for better readability
remoteproc_number=$1
firmware_name=$2

# Stop the current firmware
echo stop | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/state >/dev/null 2>&1

# Use the variables in the commands
sudo cp -f $firmware_name /lib/firmware/
echo $(basename $firmware_name) | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/firmware
echo start | sudo tee /sys/class/remoteproc/remoteproc$remoteproc_number/state
sudo cat /sys/kernel/debug/remoteproc/remoteproc$remoteproc_number/trace0


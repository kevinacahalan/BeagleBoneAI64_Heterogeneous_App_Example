#!/usr/bin/env python3

# THIS SCRIPT WILL NOT DEAL WITH CIRCULAR BUFFER OVERFLOW!

import time
import sys
import os
import subprocess

def read_trace_file(trace_file):
    if os.geteuid() == 0:
        # Running as root
        with open(trace_file, 'r') as f:
            content = f.read().splitlines()
    else:
        # Running as regular user, use sudo
        result = subprocess.run(['sudo', 'cat', trace_file], stdout=subprocess.PIPE, text=True)
        if result.returncode != 0:
            raise Exception(f"sudo cat command failed with return code {result.returncode}")
        content = result.stdout.splitlines()
    return content

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <remoteproc_number>")
    sys.exit(1)

remoteproc_id = sys.argv[1]
trace_file = f"/sys/kernel/debug/remoteproc/remoteproc{remoteproc_id}/trace0"

# Initialize last_line_count by reading the current content
try:
    content = read_trace_file(trace_file)
except Exception as e:
    print(f"Error reading trace file: {e}")
    sys.exit(1)

last_line_count = 0

# Start the monitoring loop
while True:
    try:
        content = read_trace_file(trace_file)
    except Exception as e:
        print(f"Error reading trace file: {e}")
        last_line_count = 0
        time.sleep(0.1)
        continue

    total_lines = len(content)

    if total_lines < last_line_count:
        # Buffer wrapped around or reset
        # Start fresh from current position
        last_line_count = total_lines
    elif total_lines > last_line_count:
        # Print new lines
        for line in content[last_line_count:]:
            print(line)
    # Else, no new lines

    last_line_count = total_lines

    time.sleep(0.1)

#!/usr/bin/env python3
#
# print_trace0.py - Monitor R5 firmware trace output in real-time
#
# Continuously monitors the remoteproc trace0 buffer and prints new lines
# as they appear. This is the preferred tool for watching R5 debug output.
#
# Usage: ./print_trace0.py <remoteproc_number>
#
# Note: Does not handle circular buffer wraparound detection.
#

import os
import subprocess
import sys
import time

WAIT_RETRY_SECONDS = 0.5
WAIT_LOG_SECONDS = 2.0


def read_trace_file(trace_file):
    if os.geteuid() == 0:
        # Running as root
        with open(trace_file, 'r') as file_handle:
            content = file_handle.read().splitlines()
    else:
        # Running as regular user, use sudo
        result = subprocess.run(['sudo', 'cat', trace_file], stdout=subprocess.PIPE, text=True)
        if result.returncode != 0:
            raise Exception(f"sudo cat command failed with return code {result.returncode}")
        content = result.stdout.splitlines()
    return content


def wait_for_trace_file(trace_file):
    print(f"Waiting for R5 trace buffer: {trace_file}")
    last_log_time = 0.0

    while True:
        try:
            content = read_trace_file(trace_file)
            print("R5 trace buffer is available. Monitoring trace output...")
            return content
        except Exception as exc:
            now = time.time()
            if now - last_log_time >= WAIT_LOG_SECONDS:
                print(f"Trace not ready yet ({exc}). Still waiting...")
                last_log_time = now
            time.sleep(WAIT_RETRY_SECONDS)


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <remoteproc_number>")
    sys.exit(1)

remoteproc_id = sys.argv[1]
trace_file = f"/sys/kernel/debug/remoteproc/remoteproc{remoteproc_id}/trace0"

# Initialize once trace file is available
content = wait_for_trace_file(trace_file)

last_line_count = 0

# Start the monitoring loop
try:
    while True:
        try:
            content = read_trace_file(trace_file)
        except Exception as exc:
            print(f"Trace temporarily unavailable ({exc}). Waiting for it to come back...")
            content = wait_for_trace_file(trace_file)
            last_line_count = 0
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
except KeyboardInterrupt:
    print("\nTrace monitoring stopped.")
    sys.exit(0)

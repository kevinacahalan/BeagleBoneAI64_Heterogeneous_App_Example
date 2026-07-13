#!/usr/bin/env python3
#
# print_dual_trace.py - Monitor two remoteproc trace0 buffers as one stream.
#
# Better than a blind split screen for RTU+PRU: lines are interleaved with
# labels so you can see mailbox handshakes in order over SSH.
#
# Usage:
#   ./print_dual_trace.py <rproc_a> <label_a> <rproc_b> <label_b>
#   ./print_dual_trace.py 9 RTU0_0 8 PRU0_0
#

import os
import subprocess
import sys
import time

WAIT_RETRY_SECONDS = 0.5
WAIT_LOG_SECONDS = 2.0
POLL_SECONDS = 0.1


def read_trace_file(trace_file):
    if os.geteuid() == 0:
        with open(trace_file, "r", errors="replace") as file_handle:
            return file_handle.read().splitlines()
    result = subprocess.run(
        ["sudo", "cat", trace_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode != 0:
        raise OSError(result.stderr.strip() or f"sudo cat failed ({result.returncode})")
    return result.stdout.splitlines()


class TraceSource:
    def __init__(self, remoteproc_id, label):
        self.remoteproc_id = remoteproc_id
        self.label = label
        self.path = f"/sys/kernel/debug/remoteproc/remoteproc{remoteproc_id}/trace0"
        self.last_line_count = 0
        self.ready = False

    def wait_ready(self):
        print(f"Waiting for {self.label} trace: {self.path}")
        last_log = 0.0
        while True:
            try:
                lines = read_trace_file(self.path)
                print(f"{self.label} trace ready.")
                self.ready = True
                self.last_line_count = 0
                return lines
            except Exception as exc:
                now = time.time()
                if now - last_log >= WAIT_LOG_SECONDS:
                    print(f"{self.label} not ready yet ({exc}). Still waiting...")
                    last_log = now
                time.sleep(WAIT_RETRY_SECONDS)

    def poll_new_lines(self):
        try:
            lines = read_trace_file(self.path)
        except Exception as exc:
            print(f"{self.label} temporarily unavailable ({exc}). Re-waiting...")
            self.wait_ready()
            return []

        total = len(lines)
        if total < self.last_line_count:
            self.last_line_count = total
            return []
        if total > self.last_line_count:
            new_lines = lines[self.last_line_count :]
            self.last_line_count = total
            return new_lines
        return []


def main():
    if len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <rproc_a> <label_a> <rproc_b> <label_b>")
        sys.exit(1)

    sources = [
        TraceSource(sys.argv[1], sys.argv[2]),
        TraceSource(sys.argv[3], sys.argv[4]),
    ]

    for src in sources:
        src.wait_ready()

    print("Monitoring both traces (Ctrl+C to exit)...")
    print("---")

    try:
        while True:
            for src in sources:
                for line in src.poll_new_lines():
                    print(f"[{src.label}] {line}")
            time.sleep(POLL_SECONDS)
    except KeyboardInterrupt:
        print("\nDual trace monitoring stopped.")
        sys.exit(0)


if __name__ == "__main__":
    main()

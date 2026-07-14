#!/usr/bin/env python3
"""Send a blink count to PRU0_0 over rpmsg_char and print the ACK.

On kernel 6.12 the old rpmsg_pru driver (/dev/rpmsg_pru30) is gone.
PRU firmware announces channel "rpmsg-raw" on port 30; rpmsg_char creates
/dev/rpmsgN. This script finds that device (or creates an endpoint via
/dev/rpmsg_ctrl* under the PRU remoteproc).
"""

from __future__ import annotations

import argparse
import array
import fcntl
import glob
import os
import struct
import sys
import time

CHAN_PORT = 30
RPMSG_ADDR_ANY = 0xFFFFFFFF
# linux/rpmsg.h: #define RPMSG_CREATE_EPT_IOCTL _IOW(0xb5, 0x1, struct rpmsg_endpoint_info)
RPMSG_CREATE_EPT_IOCTL = 0x4028B501  # _IOW(0xb5, 1, struct rpmsg_endpoint_info)


def _read_sysfs(path: str) -> str:
    try:
        with open(path, "r", encoding="ascii", errors="replace") as f:
            return f.read().strip()
    except OSError:
        return ""


def find_rpmsg_dev_by_dst(dst: int = CHAN_PORT) -> str | None:
    """Return /dev/rpmsgN bound to an rpmsg bus device with the given dst."""
    for bus_path in sorted(glob.glob("/sys/bus/rpmsg/devices/*")):
        raw = _read_sysfs(os.path.join(bus_path, "dst"))
        try:
            # Sysfs may be decimal ("30") or hex ("0x1e")
            if int(raw, 0) != dst:
                continue
        except ValueError:
            continue
        for node in glob.glob(os.path.join(bus_path, "rpmsg", "rpmsg*")):
            dev = f"/dev/{os.path.basename(node)}"
            if os.path.exists(dev):
                return dev
    return None


def find_pru_rpmsg_ctrl() -> str | None:
    """Find /dev/rpmsg_ctrl* owned by j7-pru0_0 / b034000.pru virtio."""
    # Walk remoteproc devices looking for name j7-pru0_0
    for rproc in glob.glob("/sys/class/remoteproc/remoteproc*"):
        name = _read_sysfs(os.path.join(rproc, "name"))
        if name not in ("j7-pru0_0", "b034000.pru"):
            # Some kernels only expose the DT node path in name
            if "pru0_0" not in name and "b034000" not in name:
                continue
        # remoteprocN/remoteprocN#vdev*/virtio*/rpmsg_ctrl*
        for ctrl in glob.glob(os.path.join(rproc, "**", "rpmsg_ctrl*"), recursive=True):
            if not os.path.isdir(ctrl):
                continue
            dev = f"/dev/{os.path.basename(ctrl)}"
            if os.path.exists(dev):
                return dev
    # Fallback: any ctrl whose sysfs path mentions b034000 / pru0
    for ctrl in glob.glob("/sys/class/rpmsg_ctrl/rpmsg_ctrl*"):
        try:
            real = os.path.realpath(ctrl)
        except OSError:
            continue
        if "b034000" in real or "pru0_0" in real or "j7-pru0_0" in real:
            dev = f"/dev/{os.path.basename(ctrl)}"
            if os.path.exists(dev):
                return dev
    return None


def create_endpoint(ctrl_dev: str, dst: int = CHAN_PORT) -> str | None:
    """Create an rpmsg_char endpoint via ioctl; return new /dev/rpmsgN path."""
    # struct rpmsg_endpoint_info { char name[32]; u32 src; u32 dst; };
    name = b"rpmsg_chrdev"
    packed = struct.pack("32sII", name, RPMSG_ADDR_ANY, dst)
    before = set(glob.glob("/dev/rpmsg[0-9]*"))
    fd = os.open(ctrl_dev, os.O_RDWR)
    try:
        fcntl.ioctl(fd, RPMSG_CREATE_EPT_IOCTL, array.array("B", packed))
    except OSError as e:
        os.close(fd)
        print(f"RPMSG_CREATE_EPT_IOCTL on {ctrl_dev} failed: {e}", file=sys.stderr)
        return None
    # Keep ctrl fd open until endpoint is found (some kernels need it)
    deadline = time.monotonic() + 2.0
    dev = None
    while time.monotonic() < deadline:
        after = set(glob.glob("/dev/rpmsg[0-9]*"))
        new = after - before
        if new:
            dev = sorted(new)[0]
            break
        # Also try dst-based discovery
        dev = find_rpmsg_dev_by_dst(dst)
        if dev:
            break
        time.sleep(0.05)
    os.close(fd)
    return dev


def resolve_device(explicit: str | None, timeout: float) -> str | None:
    if explicit:
        return explicit if os.path.exists(explicit) else None

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        dev = find_rpmsg_dev_by_dst(CHAN_PORT)
        if dev:
            return dev
        ctrl = find_pru_rpmsg_ctrl()
        if ctrl:
            created = create_endpoint(ctrl, CHAN_PORT)
            if created:
                return created
        time.sleep(0.1)
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("count", type=int, help="Number of LED blinks (P8_11)")
    parser.add_argument(
        "-d",
        "--device",
        default=None,
        help="RPMsg char device (default: auto-discover port 30)",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=float,
        default=30.0,
        help="Seconds to wait for ACK (default: 30)",
    )
    parser.add_argument(
        "--discover-timeout",
        type=float,
        default=5.0,
        help="Seconds to wait for /dev/rpmsgN (default: 5)",
    )
    args = parser.parse_args()

    if args.count < 1:
        print("count must be >= 1", file=sys.stderr)
        return 1

    device = resolve_device(args.device, args.discover_timeout)
    if not device:
        print(
            "No PRU RPMsg device found (expected rpmsg-raw port 30).\n"
            "Start firmware first:\n"
            "  sudo ./extra-examples/pru0_0-rpmsg-led/scripts/run.sh start\n"
            "Ensure the overlay has &pru0_0 vring IRQ and was rebooted.",
            file=sys.stderr,
        )
        return 1

    print(f"Using {device}", file=sys.stderr)
    msg = f"{args.count}\n".encode("ascii")
    try:
        fd = os.open(device, os.O_RDWR | os.O_NONBLOCK)
    except PermissionError:
        print(
            f"Permission denied opening {device}.\n"
            "Re-run with sudo, for example:\n"
            f"  sudo python3 extra-examples/pru0_0-rpmsg-led/scripts/blink_count.py {args.count}",
            file=sys.stderr,
        )
        return 1
    try:
        os.write(fd, msg)
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            try:
                data = os.read(fd, 64)
                if data:
                    sys.stdout.write(data.decode("ascii", errors="replace"))
                    return 0
            except BlockingIOError:
                time.sleep(0.05)
        print("timeout waiting for PRU ACK", file=sys.stderr)
        return 1
    finally:
        os.close(fd)


if __name__ == "__main__":
    sys.exit(main())

# RTU0_0_SIDE — ICSSG0 RTU0 firmware

Firmware for BeagleBone AI-64 **RTU0_0** (`j7-rtu0_0` / `b004000.rtu` / DT `&rtu0_0`).

Used with [`PRU0_0_SIDE`](../PRU0_0_SIDE/) `led_worker` for the cooperative LED demo:
RTU owns RPMsg; PRU blinks P8_11 from shared DMEM (`ICSSG0_SHARED/include/led_mailbox.h`).

## App

| App | Firmware | Purpose |
|---|---|---|
| `rpmsg_led` | `build/RTU0_0/rtu0_0-rpmsg-led.out` | Receive blink count over `rpmsg-raw` port 30; post to mailbox; ACK after PRU finishes |

## Build

```bash
./scripts/build.sh --setup   # once
./scripts/build.sh --pru     # builds PRU0_0 apps + this RTU firmware
```

## Board

Overlay must include `&rtu0_0` vring `<20 4 4>` (see
[`custom_overlays/our-custom-bbai64-overlay.dtso`](../custom_overlays/our-custom-bbai64-overlay.dtso)).
Rebuild/install overlay and reboot after adding it.

```bash
sudo ./scripts/debug_rtu_pru_led.sh start
sudo ./scripts/debug_rtu_pru_led.sh trace   # merged [RTU0_0]/[PRU0_0] lines
# or: sudo ./scripts/debug_rtu_pru_led.sh trace-split   # tmux panes
sudo python3 PRU0_0_SIDE/host/blink_count.py 5
sudo ./scripts/debug_rtu_pru_led.sh stop
```

Do **not** run phase-1 `debug_pru0_0.sh start rpmsg_led` at the same time — both
claim RPMsg port 30.

Resolve remoteprocs by name (`j7-rtu0_0`, `j7-pru0_0`); do not hardcode `remoteprocN`.

## Kernel 6.12 notes

Unlike older 5.10 tutorials (e.g. Loic example-05):

- Channel name is `rpmsg-raw` → `/dev/rpmsgN` via `rpmsg_char` (not `rpmsg-pru` / `/dev/rpmsg_pru30`)
- Resource table size must be a multiple of 16 bytes
- DT must supply the RTU vring IRQ

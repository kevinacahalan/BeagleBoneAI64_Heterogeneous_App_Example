# rtu0_0-pru0_0-rpmsg-led

Cooperative **Linux ↔ RTU0_0 ↔ PRU0_0** LED demo.

- **RTU0_0** owns RPMsg (`rpmsg-raw` port 30), posts blink count to a shared DMEM mailbox, ACKs after PRU finishes.
- **PRU0_0** `led_worker` polls the mailbox and blinks P8_11.

## Layout

```
RTU0_0_SIDE/                  RTU rpmsg_led firmware
PRU0_0_SIDE/                  PRU led_worker firmware
ICSSG0_SHARED/                led_mailbox.h (shared DMEM protocol)
scripts/run.sh
scripts/blink_count.py
```

Firmware outputs under `build/extra-examples/rtu0_0-pru0_0-rpmsg-led/`:

- `rtu0_0-rpmsg-led.elf`
- `pru0_0-led-worker.elf`

## Build

```bash
./scripts/build.sh --setup
./scripts/build.sh --extra rtu0_0-pru0_0-rpmsg-led
```

## Run on the board

Overlay must include `&rtu0_0` vring `<20 4 4>` and P8_11 pinmux — see
[`custom_overlays/our-custom-bbai64-overlay.dtso`](../../custom_overlays/our-custom-bbai64-overlay.dtso).

```bash
sudo ./extra-examples/rtu0_0-pru0_0-rpmsg-led/scripts/run.sh demo      # start + blink 5 times
sudo ./extra-examples/rtu0_0-pru0_0-rpmsg-led/scripts/run.sh demo 10   # custom count
sudo ./extra-examples/rtu0_0-pru0_0-rpmsg-led/scripts/run.sh trace   # merged [RTU0_0]/[PRU0_0]
# or: sudo ./extra-examples/rtu0_0-pru0_0-rpmsg-led/scripts/run.sh trace-split
sudo ./extra-examples/rtu0_0-pru0_0-rpmsg-led/scripts/run.sh stop
```

Do **not** run [`pru0_0-rpmsg-led`](../pru0_0-rpmsg-led/) at the same time — both
claim RPMsg port 30.

Resolve remoteprocs by name (`j7-rtu0_0`, `j7-pru0_0`).

## Kernel 6.12 notes

- Channel name is `rpmsg-raw` → `/dev/rpmsgN` via `rpmsg_char`
- Resource table size must be a multiple of 16 bytes
- DT must supply the RTU vring IRQ

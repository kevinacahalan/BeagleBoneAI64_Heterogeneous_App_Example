# pru0_0-rpmsg-led

**Linux ↔ PRU0_0** LED blink over RPMsg.

PRU firmware listens on `rpmsg-raw` port **30**, blinks P8_11 (`__R30` bit 17)
that many times, and ACKs. The host script sends an ASCII count over `/dev/rpmsgN`.

## Layout

```
PRU0_0_SIDE/                  firmware
LINUX_SIDE/host/blink_count.py
scripts/run.sh
```

Firmware output: `build/extra-examples/pru0_0-rpmsg-led/pru0_0-rpmsg-led.elf`

## Build

```bash
./scripts/build.sh --setup
./scripts/build.sh --extra pru0_0-rpmsg-led
```

## Run on the board

Connect an LED to **P8_11**. Overlay must mux that pin and provide `&pru0_0`
vring IRQ (`<16 2 2>`) — see
[`custom_overlays/our-custom-bbai64-overlay.dtso`](../../custom_overlays/our-custom-bbai64-overlay.dtso).

```bash
sudo ./extra-examples/pru0_0-rpmsg-led/scripts/run.sh start
sudo python3 extra-examples/pru0_0-rpmsg-led/LINUX_SIDE/host/blink_count.py 5
sudo ./extra-examples/pru0_0-rpmsg-led/scripts/run.sh trace
sudo ./extra-examples/pru0_0-rpmsg-led/scripts/run.sh stop
```

Do **not** run [`rtu0_0-pru0_0-rpmsg-led`](../rtu0_0-pru0_0-rpmsg-led/) at the
same time (both claim port 30).

## RPMsg protocol

- Channel: `rpmsg-raw`, port `30` → `/dev/rpmsgN` via `rpmsg_char`
- Host writes ASCII decimal count + newline; firmware blinks and ACKs
- `/dev/rpmsgN` is root-owned (`0600`); use `sudo` for the host script

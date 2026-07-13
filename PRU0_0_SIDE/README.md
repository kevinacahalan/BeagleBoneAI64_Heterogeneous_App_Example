# PRU0_0_SIDE — ICSSG0 PRU0 firmware

Firmware for BeagleBone AI-64 **PRU0_0** (`j7-pru0_0` / `b034000.pru` / DT `&pru0_0`).

## Apps

| App | Firmware | Linker | Purpose |
|---|---|---|---|
| `hello` | `build/PRU0_0/pru0_0-hello.out` | `J721E_PRU0_0.cmd` (no `.pru_irq_map`) | Writes “Hello world…” to remoteproc `trace0` |
| `rpmsg_led` | `build/PRU0_0/pru0_0-rpmsg-led.out` | `J721E_PRU0_0_rpmsg.cmd` (`.pru_irq_map (COPY)`) | Blink P8_11 N times when Linux sends N over RPMsg |
| `led_worker` | `build/PRU0_0/pru0_0-led-worker.out` | `J721E_PRU0_0_worker.cmd` | Blink P8_11 from shared mailbox (pair with RTU0_0) |

Hello / `led_worker` must **not** carry an empty `.pru_irq_map` section — kernel 6.12 rejects that with `header-less .pru_irq_map section` / `Boot failed: -22`.

The resource table section size must be a **multiple of 16 bytes** (padding after the real entries). Otherwise `rproc_start`’s ARM64 `memcpy` into ICSSG device memory can hit an alignment fault.

## Build (Docker / Podman — required path)

Uses the same TI image as R5 (`docker/Dockerfile.ti` with `clpru`):

```bash
# Once: fetch/build TI SDK+PDK and PSSP under ~/ti
./scripts/build.sh --setup

# Then build PRU0_0 + RTU0_0 firmware
./scripts/build.sh --pru
```

`--pru` does not fetch PSSP (same as `--r5` does not fetch the SDK). PSSP lives at
`~/ti/pru-software-support-package/`. Outputs land in `build/PRU0_0/` and `build/RTU0_0/`.

## Pinmux

P8_11 = `PRG0_PRU0_GPO17` = `__R30` bit 17, muxed in
[`custom_overlays/our-custom-bbai64-overlay.dtso`](../custom_overlays/our-custom-bbai64-overlay.dtso)
as SysConfig GPO: `J721E_IOPAD(0xf4, PIN_INPUT, 0)`.

Rebuild/install the overlay and reboot before testing the LED / RPMsg apps.
The overlay must also provide `&pru0_0` **vring** IRQ cells (`<16 2 2>` via
`&icssg0_intc`); kernel 6.12 j721e DT omits them and `rpmsg_led` will fail with
`IRQ vring not found` / `Boot failed: -6`. Verify pinmux with
`sudo ./scripts/show-pins.pl` (mode nibble should be `0`).

## Load on the board

### Phase-1 single-core demos

```bash
sudo ./scripts/debug_pru0_0.sh start hello
sudo ./scripts/debug_pru0_0.sh trace

sudo ./scripts/debug_pru0_0.sh start rpmsg_led
sudo python3 PRU0_0_SIDE/host/blink_count.py 5
```

### RTU + PRU cooperative LED (additional demo)

See [`RTU0_0_SIDE/README.md`](../RTU0_0_SIDE/README.md). Short version:

```bash
sudo ./scripts/debug_rtu_pru_led.sh start
sudo ./scripts/debug_rtu_pru_led.sh trace
sudo python3 PRU0_0_SIDE/host/blink_count.py 5
```

Do not run phase-1 `rpmsg_led` and the RTU demo at the same time (both use port 30).

`/dev/rpmsgN` is root-owned (`0600`); use `sudo` for the host script (or open the device as root).

Always resolve the remoteproc instance by name (`j7-pru0_0`). On kernel 6.12
it is often `remoteproc8`, which differs from older 5.10 tutorial numbering.

## RPMsg protocol

- Channel name: `rpmsg-raw`, port `30` → `/dev/rpmsgN` via `rpmsg_char`
  (kernel 6.12 has no `rpmsg_pru` / `/dev/rpmsg_pru30`)
- Host writes ASCII decimal count + newline; firmware blinks that many times and ACKs
- Phase 1: PRU `rpmsg_led` handles RPMsg + blink
- Cooperative: RTU handles RPMsg; PRU `led_worker` blinks via
  [`ICSSG0_SHARED/include/led_mailbox.h`](../ICSSG0_SHARED/include/led_mailbox.h)

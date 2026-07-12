# PRU0_0_SIDE — ICSSG0 PRU0 firmware

Firmware for BeagleBone AI-64 **PRU0_0** (`j7-pru0_0` / `b034000.pru` / DT `&pru0_0`).

## Apps

| App | Firmware | Linker | Purpose |
|---|---|---|---|
| `hello` | `build/PRU0_0/pru0_0-hello.out` | `J721E_PRU0_0.cmd` (no `.pru_irq_map`) | Writes “Hello world…” to remoteproc `trace0` |
| `rpmsg_led` | `build/PRU0_0/pru0_0-rpmsg-led.out` | `J721E_PRU0_0_rpmsg.cmd` (`.pru_irq_map (COPY)`) | Blink P8_11 N times when Linux sends N over RPMsg |

Hello must **not** carry an empty `.pru_irq_map` section — kernel 6.12 rejects that with `header-less .pru_irq_map section` / `Boot failed: -22`.

The resource table section size must be a **multiple of 16 bytes** (padding after the real entries). Otherwise `rproc_start`’s ARM64 `memcpy` into ICSSG device memory can hit an alignment fault.

## Build (Docker / Podman — required path)

Uses the same TI image as R5 (`docker/Dockerfile.ti` with `clpru`):

```bash
# Once: fetch/build TI SDK+PDK and PSSP under ~/ti
./scripts/build.sh --setup

# Then build PRU0_0 firmware only
./scripts/build.sh --pru
```

`--pru` does not fetch PSSP (same as `--r5` does not fetch the SDK). PSSP lives at
`~/ti/pru-software-support-package/`. Outputs land in `build/PRU0_0/`.

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

```bash
# After rsync / deploy of build/PRU0_0/
sudo ./scripts/debug_pru0_0.sh start hello
sudo ./scripts/debug_pru0_0.sh trace

sudo ./scripts/debug_pru0_0.sh start rpmsg_led
python3 PRU0_0_SIDE/host/blink_count.py 5
```

Always resolve the remoteproc instance by name (`j7-pru0_0`). On kernel 6.12
it is often `remoteproc8`, which differs from older 5.10 tutorial numbering.

## RPMsg protocol (phase 1)

- Channel name: `rpmsg-raw`, port `30` → `/dev/rpmsgN` via `rpmsg_char`
  (kernel 6.12 has no `rpmsg_pru` / `/dev/rpmsg_pru30`)
- Host writes ASCII decimal count + newline; PRU blinks that many times and ACKs

## Phase 2 roadmap — `RTU0_0_SIDE/`

Like [loic’s example-05](https://github.com/loic-fejoz/beaglebone-ai64-tutorial/tree/feat/fft/example-05-pru-dds-rtu):

1. Add `RTU0_0_SIDE/` for ICSSG0 RTU0 (`j7-rtu0_0` / `b004000.rtu`)
2. Move RPMsg virtio endpoint onto the RTU
3. Keep pin timing / LED (or DDS) on `PRU0_0_SIDE`, coordinated via ICSSG shared DMEM

Phase 1 keeps RPMsg on the PRU so the first demos stay single-core.

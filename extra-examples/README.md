# Extra examples

Standalone demos beyond the main **Linux ↔ R5F0_0** app at the repo root.

| Demo | What it shows | Run |
|------|---------------|-----|
| [`pru0_0-hello`](pru0_0-hello/) | PRU0_0 writes hello to remoteproc `trace0` | `sudo ./extra-examples/pru0_0-hello/scripts/run.sh start` then `trace` |
| [`pru0_0-rpmsg-led`](pru0_0-rpmsg-led/) | Linux sends blink count to PRU0_0 over RPMsg; LED on P8_11 | `sudo ./extra-examples/pru0_0-rpmsg-led/scripts/run.sh demo` |
| [`rtu0_0-pru0_0-rpmsg-led`](rtu0_0-pru0_0-rpmsg-led/) | Linux → RTU0_0 RPMsg → PRU0_0 blink via shared DMEM | `sudo ./extra-examples/rtu0_0-pru0_0-rpmsg-led/scripts/run.sh demo` |

## Build

From the repo root (after `./scripts/build.sh --setup`):

```bash
./scripts/build.sh --extras                          # all three demos, all sides
./scripts/build.sh --extras --pru                     # PRU sides only
./scripts/build.sh --extra pru0_0-hello
./scripts/build.sh --extra pru0_0-rpmsg-led --pru
./scripts/build.sh --extra rtu0_0-pru0_0-rpmsg-led --rtu
```

Outputs land under `build/extra-examples/<demo-name>/`.

`--main` builds only the main Linux + R5 demo. `--all` builds the main demo **and** these extras.
Side filters `--linux` / `--r5` / `--pru` / `--rtu` must be paired with a primary target.

## Notes

- The two LED demos both use RPMsg port **30** — do not run them at the same time.
- Board pinmux / vring IRQs come from the shared overlay:
  [`custom_overlays/our-custom-bbai64-overlay.dtso`](../custom_overlays/our-custom-bbai64-overlay.dtso).
- Each demo owns its own `PRU0_0_SIDE` / `RTU0_0_SIDE` pieces (and host scripts under `scripts/`).

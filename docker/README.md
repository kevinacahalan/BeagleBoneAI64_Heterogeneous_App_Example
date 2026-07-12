# Container images for BeagleBone AI64 heterogeneous builds

This project uses **two** container images. Each targets a different part of the toolchain.

## Debian 13 — Linux builds

| | |
|---|---|
| **Image** | `localhost/debian13-bbai64-build:latest` |
| **Dockerfile** | [`Dockerfile.debian13`](Dockerfile.debian13) |
| **Base** | `debian:13` |
| **Used for** | Linux aarch64 cross-build (`--linux`, `--all`) |

Debian 13 provides `libgpiod-dev:arm64` (gpiod v2 API) required by the Linux application. Do not move Linux builds to the TI image.

## TI Ubuntu — R5 / PRU / TI SDK/PDK / TI PSSP builds

| | |
|---|---|
| **Image** | `localhost/ti-bbai64-build:latest` |
| **Dockerfile** | [`Dockerfile.ti`](Dockerfile.ti) |
| **Base** | [`ghcr.io/texasinstruments/ubuntu-distro:latest`](https://github.com/TexasInstruments/ti-docker-images) (pin a digest in `Dockerfile.ti` if you need bit-for-bit reproducible image builds) |
| **Used for** | R5 firmware, PRU0_0 firmware (`clpru`), SDK/PDK setup, PSSP fetch + `rpmsg_lib` (`--r5`, `--pru`, `--setup`, `--all`) |

The TI image matches TI's recommended environment for Processor SDK / Yocto-style builds. The SDK tarball (~3 GB) is **not** baked into the image; mount `~/ti` from the host. PRU CGT 2.3.3 is installed into the image at build time.

## Quick reference

```bash
# Show help (no args does not build)
./scripts/build.sh

# One-time deps: TI SDK+PDK and PSSP+rpmsg_lib
./scripts/build.sh --setup

# Firmware only (after --setup)
./scripts/build.sh --linux
./scripts/build.sh --r5
./scripts/build.sh --pru
./scripts/build.sh --all      # Linux + R5 + PRU
```

See the main [README](../README.md) for full build instructions.

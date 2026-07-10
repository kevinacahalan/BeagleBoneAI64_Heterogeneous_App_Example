# Container images for BeagleBone AI64 heterogeneous builds

This project uses **two** container images. Each targets a different part of the toolchain.

## Debian 13 — Linux builds

| | |
|---|---|
| **Image** | `localhost/debian13-bbai64-build:latest` |
| **Dockerfile** | [`Dockerfile.debian13`](Dockerfile.debian13) |
| **Base** | `debian:13` |
| **Used for** | Linux aarch64 cross-build (`--linux`, `--both`) |

Debian 13 provides `libgpiod-dev:arm64` (gpiod v2 API) required by the Linux application. Do not move Linux builds to the TI image.

## TI Ubuntu — R5 / PDK builds

| | |
|---|---|
| **Image** | `localhost/ti-bbai64-build:latest` |
| **Dockerfile** | [`Dockerfile.ti`](Dockerfile.ti) |
| **Base** | [`ghcr.io/texasinstruments/ubuntu-distro`](https://github.com/TexasInstruments/ti-docker-images) |
| **Used for** | R5 firmware, SDK download, PDK `all_libs` (`--r5`, `--setup`) |

The TI image matches TI's recommended environment for Processor SDK / Yocto-style builds. The SDK tarball (~3 GB) is **not** baked into the image; mount `~/ti` from the host.

## Quick reference

```bash
# Build both images (happens automatically on first build)
./scripts/docker_cross_build.sh --linux    # Debian 13 only
./scripts/docker_cross_build.sh --r5       # TI image only
./scripts/docker_cross_build.sh --both     # Debian 13, then TI

# One-time SDK + PDK setup (TI container)
./scripts/docker_cross_build.sh --setup
```

See the main [README](../README.md) for full build instructions.

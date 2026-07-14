# pru0_0-hello

Minimal **PRU0_0** (`j7-pru0_0`) firmware that writes a string into the remoteproc
`trace0` buffer and halts.

## Layout

```
PRU0_0_SIDE/     firmware sources + Makefile
scripts/run.sh   start / stop / trace on the board
```

Firmware output: `build/extra-examples/pru0_0-hello/pru0_0-hello.elf`

## Build

```bash
./scripts/build.sh --setup                 # once
./scripts/build.sh --extra pru0_0-hello
```

## Run on the board

```bash
sudo ./extra-examples/pru0_0-hello/scripts/run.sh start
sudo ./extra-examples/pru0_0-hello/scripts/run.sh trace
sudo ./extra-examples/pru0_0-hello/scripts/run.sh stop
```

Always resolve remoteproc by name (`j7-pru0_0`); do not hardcode `remoteprocN`.

## Notes

- Linker script has **no** empty `.pru_irq_map` section (kernel 6.12 rejects that).
- Resource table size is padded to a multiple of 16 bytes for safe `rproc_start` memcpy.

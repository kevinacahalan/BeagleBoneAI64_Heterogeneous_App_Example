# third_party/

Dependencies fetched by the build scripts (not committed to git).

## PRU Software Support Package (PSSP)

Path: `pru-software-support-package/`

Fetched and built automatically when you run:

```bash
./scripts/build.sh --setup
```

That clones a pinned commit (see `scripts/lib/pssp_config.sh`) and builds
`lib/rpmsg_lib.lib` with `clpru` inside the TI container. After setup:

```bash
./scripts/build.sh --pru
```

Manual steps (normally unnecessary):

```bash
# Inside the TI container, or with PRU_CGT on PATH:
./scripts/lib/fetch_pssp.sh
./scripts/lib/build_pssp_lib.sh
```

Sources:

- Mirror: https://github.com/dinuxbg/pru-software-support-package
- Upstream: https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/

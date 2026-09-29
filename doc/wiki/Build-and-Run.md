# Build and Run

## Prerequisites

- Linux or WSL2 (the project is developed on WSL2; macOS should work but is less tested)
- CMake ≥ 3.14
- A C++17 compiler (GCC 9+ recommended)
- `libsqlite3-dev`, `libboost-dev` (header-only Boost is fine)
- Network access at first build — CMake's `ExternalProject_Add` clones [`hivcdm/cepac-transm`](https://github.com/hivcdm/cepac-transm), [`pugixml`](https://github.com/hivcdm/pugixml), and a couple of other deps from GitHub.

## First build

```bash
git clone git@github.com:hivcdm/transm.git
cd transm
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

The binary lands in `transm/bin/transm-vX.Y.Z` where `X.Y.Z` is the contents of `transm/VERSION`. Static archives (`libcepac.a`, `libpugixml.a`) live in `transm/lib/`.

## Running a simulation

A run needs **two things**:

1. An XML scenario file (e.g. `focus_test.xml`)
2. A directory of CEPAC `.in` files referenced from inside the XML

```bash
cd /home/samanseifi/codes/hiv-CDM/MIAMI-prime
../transm/bin/transm-v4.8.0 --cepac INFILES focus_test.xml
```

A monthly progress bar prints. Outputs land alongside the input XML, named `<sim_name>-<traceFile>.xls` for each enabled trace file plus a few summary files.

## Common flags

- `--cepac <dir>` — directory containing the CEPAC `.in` files referenced by the XML.
- `--focus` — enable the FOCUS screening module (set in [`EventParams::focusEnabled`](https://github.com/hivcdm/transm/blob/develop/src/parameters/eventparams.hpp#L170)).
- See the source of [`transm.cpp`](https://github.com/hivcdm/transm/blob/develop/src/transm.cpp) for the full CLI.

## Versioning

`VERSION` is the source of truth. CMake reads it at configure time and regenerates [`src/utility/version.h`](https://github.com/hivcdm/transm/blob/develop/src/utility/version.h) via `version_header.sh`. Bumping `VERSION` and reconfiguring is the right way to cut a new release.

## Troubleshooting

**`fatal: could not read Username for 'https://github.com'` during build** — git's credential helper is misconfigured (often a stale `gh` path). Easiest workaround: configure the remotes (transm and cepac-transm) to use SSH:
```bash
git remote set-url origin git@github.com:hivcdm/transm.git
```

**Build fails on cepac source** — the CMake `ExternalProject_Add(cepac_project)` re-clones cepac on every configure. If the clone failed (e.g. offline) but you have the cepac source locally, copy `/path/to/cepac-transm/src` into `build/third-party/src/cepac_project/src` and re-run `make`.

**Segfault at end of simulation on v4.8.0 tag** — this was a double-free in `EventParams::~EventParams()`, fixed on `develop` after the tag. Pull the latest `develop` and rebuild.

# Third-Party Notices

TRANSM's own source code is licensed under the GNU General Public License,
version 3 or later (see [LICENSE](LICENSE) and [NOTICE](NOTICE)). The components
below are **not** covered by that license. Each remains under its own license,
which governs your use of it.

All of them except CEPAC are under licenses compatible with the GPL. CEPAC is not
free software; [NOTICE](NOTICE) grants the additional permission that lets TRANSM
be linked with it and distributed.

## Included in this repository

| File | Copyright | License |
|---|---|---|
| `cmake/FindSQLITE3.cmake` | Copyright (C) 2007-2009 LuaDist | MIT (stated in the file header) |
| `cmake/FindGperftools.cmake` | not stated | no license header; origin not recorded |

## Fetched at build time

These are downloaded by CMake during the build. They are not stored in this
repository.

| Component | Source | License |
|---|---|---|
| rana | [hivcdm/rana](https://github.com/hivcdm/rana), fork of tfussell/rana | MIT |
| TCLAP | [hivcdm/tclap](https://github.com/hivcdm/tclap), fork of evanmoran/tclap | MIT |
| pugixml | [hivcdm/pugixml](https://github.com/hivcdm/pugixml), fork of zeux/pugixml | MIT |
| GoogleTest | [google/googletest](https://github.com/google/googletest) — tests only | BSD-3-Clause |
| CEPAC | [hivcdm/cepac-transm](https://github.com/hivcdm/cepac-transm) | licensed separately; not free software |

## System libraries

TRANSM links against these when building. You install them separately; they are
not distributed with TRANSM.

| Library | License |
|---|---|
| Boost | Boost Software License 1.0 |
| SQLite | Public domain |
| gperftools (optional, profiling) | BSD-3-Clause |
| OpenMP | provided by your compiler's runtime |

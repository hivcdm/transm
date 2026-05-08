# Developer's Guide

For developers joining the project, fixing bugs, or adding features. Pairs with the [Architecture Overview](Architecture-Overview.md) — that's the *map*; this is *how to drive*.

## Build basics

See [Build and Run](Build-and-Run.md) for the prerequisites. The first build clones cepac, pugixml, etc. as `ExternalProject_Add` dependencies. Subsequent builds are incremental.

### Tips for fast iteration

- Build only the main target: `make transm` (skip tests target).
- For changes to cepac: edit in `cepac-transm/`, run `make` there to produce a new `libcepac.a`, copy it to `transm/lib/libcepac.a`, then `make transm` in transm's `build/`. The incremental relink takes seconds.
- For changes to a single transm file: just `make transm` — CMake handles incremental rebuilds.
- If `cmake .` reconfigures and breaks the cepac source dir, repopulate it from sibling repo: `cp -r ../cepac-transm/src build/third-party/src/cepac_project/`.

## Code conventions

- C++17. `nullptr`, `auto`, range-based for, `std::array` over C arrays in new code.
- Indentation is mixed (tabs in older files, spaces in newer). Match the file you're editing; don't reformat unrelated code in the same PR.
- Header guards: `#pragma once` is standard; older files may have `#ifndef ... #define ...` guards.
- No `using namespace std;` at file scope. Prefix with `std::` everywhere except where the surrounding code already uses unqualified `cout`/`endl`.
- Avoid leaking new classes into the global namespace; everything transm should be `namespace transm { ... }`.

## Codebase tour

| If you're touching… | Start here |
|---|---|
| Partnership logic | [`src/entities/male.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp), [`src/core/population.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/core/population.cpp) `UpdatePartnerships` & `CreatePartnerships` |
| Transmission probability | `Male::getFOI`, `Female::getFOI`, the FOI block in [`src/entities/entity.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/entities/entity.cpp#L721) |
| Adding a new XML parameter | [`src/parameters/simulationparametersxml.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp) — find the section that parses similar parameters; copy the pattern |
| Adding a new intervention | `KnownIntervention` enum + the giant switch in `simulationparametersxml.cpp` (search for `KnownIntervention::CepacContext` for an example) |
| Adding a new output column | The relevant tracker file in [`src/statistics/`](https://github.com/hsphcdm/transm/tree/develop/src/statistics) — increment a counter in `recordIncidentInfection`, add a header line, add an output line in `printInfections` (or its sibling) |
| CEPAC interaction | `Entity::cepacPatient` in [`src/entities/entity.hpp`](https://github.com/hsphcdm/transm/blob/develop/src/entities/entity.hpp) and the CEPAC headers in `cepac-transm/src/` |
| Random numbers | [`src/utility/randomnumbergenerator.hpp`](https://github.com/hsphcdm/transm/blob/develop/src/utility/randomnumbergenerator.hpp) — wrappers over `std::mt19937` |

## Worked example: adding a new parameter

Suppose you want a new XML parameter `<myMultiplier>` under `<populationParameters>` that gets used somewhere in `Male::getFOI`. The 5-step recipe:

1. **Declare it** as a member of [`PopulationParameters`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/populationparameters.hpp).
2. **Parse it** in [`simulationparametersxml.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp) — find `GetPopulationParameters` and add `Text<double>(node.child("myMultiplier"))` next to similar-shaped params.
3. **Plumb access** with a getter on `PopulationParameters` (`double getMyMultiplier() const { return myMultiplier_; }`).
4. **Use it** in `Male::getFOI`. Multiply into the formula at the appropriate place.
5. **Document it** — update the [Input XML Reference](Input-XML-Reference.md) page.

If you're adding it as an intervention (changeable mid-simulation), there's also a dispatch entry to add in `KnownIntervention` and the switch.

## Worked example: adding a new output column

Suppose you want a new "Cumulative Incident MSM" column in the `Infections.xls` trace.

1. **Add a counter** to [`InfectionsTracker`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/infectionstracker.hpp). Default-initialize it: `OrientationArray<std::size_t> totalIncidentInfsOrientation_msmOnly{};` (note the `{}` — see [v4.8 lessons](#v48-lessons-learned)).
2. **Increment it** inside `InfectionsTracker::recordIncidentInfection` when `infectedOrientation == Msm`.
3. **Reset it** in `Initialize()` (cumulative — never reset) or `resetIncidentInfections` (monthly — reset each month). Match the lifecycle to similar counters.
4. **Add the header** in `printInfections` for time 0 — the firstRow/secondRow/thirdRow stream additions.
5. **Add the data line** in `printInfections` body — `_outStream << myCounter << Constants::Tab;`.
6. **Verify** the column count matches between header and body (this is a regular source of misalignment bugs).

## v4.8 lessons learned

These are real bugs that shipped and were fixed. Read them before writing new tracker code.

### 1. Always value-initialize `std::array` locals
A local `GenderArray<std::size_t> incidentInfsGender;` (no `{}`) leaves the elements **uninitialized**. They contain whatever was on the stack from prior calls. Then `incidentInfsGender[gender] += value;` reads garbage and adds to it. The bug is silent at small populations because the stack happens to be zero, but appears at large populations as a small persistent offset between New Infections and Male+Female totals. Always write `{}`:
```cpp
GenderArray<std::size_t> incidentInfsGender{};   // ← do this
```

### 2. Don't pass `_infector` where you mean `_infected`
`recordIncidentInfection` had `infectedGender = _infector->getDemographicProfileVal<Gender>()` for years. The bug only surfaced when someone tried to compute heterosexual male-only counts and saw they were 100% off. Read every parameter carefully when copy-pasting tracker code.

### 3. Be careful with `delete` of pointers that might alias
`untreatedContext` and `treatedContext` looked like ownership pointers in `EventParams`, so the destructor was extended to `delete` them. Turns out they're aliased to `RolloutContext::rolloutSimContext` after rollout fires — leading to a double-free at process exit. Lesson: if you're tempted to add a `delete X` in a destructor, first prove that `X` is owned uniquely.

### 4. `isWhite()` doesn't exclude Hispanics
"White Hispanic" was being counted in BOTH the white pool and the Hispanic pool by FOCUS find functions. Race and ethnicity are orthogonal axes. If you write a filter, decide explicitly: do I want race-only, ethnicity-only, or the combination?

## Random number generator and reproducibility

`EventParams::randomNums` is a single `RandomNumberGenerator` (mt19937 wrapper) used everywhere. **Adding a new random draw shifts the seed for every subsequent draw**, which can change a calibration that was matching targets. If you add randomness:

- Note it in the commit message.
- Run a calibration check before merging.
- Consider whether the new randomness can use a dedicated sub-RNG seeded from the main one (as `BetaDist::FromNormal` does).

## Testing

The `tests/` directory has a few test fixtures, but the primary way changes are validated is **running the calibration scenarios in `runs/`** and comparing outputs to expected/`.out` references. There is no unit-test culture in this codebase yet — that's a TODO target.

If you're touching the FOI calculation, partnership formation, or anything with statistical effect, run at least one MIAMI scenario and eyeball the prevalence/incidence/transmission curves vs. expected.

## Pull request workflow

- Branch from `develop`. Name it `IssueN` or `fix-foo` or `feature-bar`.
- Open the PR against `develop`. Include the issue number in the title.
- For releases, bump `VERSION`, update `CHANGELOG`, and update the `v4.x` ASCII art in `README.md` and `CHANGELOG`. CMake regenerates `version.h` from `VERSION` on configure.
- Tags are annotated tags on `develop` after merge: `git tag -a vX.Y.Z -m "..." && git push origin vX.Y.Z`.

## Refactor wishlist (from `TODO`)

- **CEPAC adapter** (`CepacPatientView`) — single source of truth for HVL/CD4 enum mappings, hides CEPAC headers from the rest of transm.
- **Split `population.cpp`** (3800+ lines) into `Population` (container only), `PartnershipManager`, `InterventionRunner`.
- **Collapse `infectionstracker`'s ~30 parallel counters** into an event log + lazy aggregations.
- **Merge Male / Female / MSM / MSMW** into one polymorphic `Entity` with composable behavior modules — would eliminate the ~50 stub virtual methods on `Entity`.
- **Parameterize the hardcoded race-specific coital multipliers** (currently in `Male::rollNumEventsPerPartner`, Issue #91).

None of these are starter tasks. The CEPAC adapter is probably the highest-leverage one if you want a chunky project.

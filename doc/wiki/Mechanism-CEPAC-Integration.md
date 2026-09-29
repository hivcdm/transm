# Mechanism: CEPAC Integration

How `transm` couples to [CEPAC](https://github.com/hivcdm/cepac-transm) (the disease-progression model). Understanding this boundary is essential for anyone modifying disease state, ART rollout, or mortality.

## The split

| | What it models | Where it lives |
|---|---|---|
| **transm** | Sexual partnerships, transmission, demographics, interventions, screening | This repo |
| **CEPAC** | Per-patient disease progression (CD4, HVL, OIs), ART efficacy, mortality | Sister repo `hivcdm/cepac-transm` |

`transm` is the **outer loop**: it advances simulation time, manages the population, decides who has sex with whom, and rolls transmission dice. CEPAC is the **inner clinical engine**: each Entity carries a CEPAC `Patient`, and once a month transm hands control to CEPAC to advance that patient's clinical state.

## Build-time coupling

CEPAC is a separate C++ project linked statically as `libcepac.a`. From [`CMakeLists.txt`](https://github.com/hivcdm/transm/blob/develop/CMakeLists.txt#L101-L117):

```cmake
ExternalProject_Add(
    cepac_project
    GIT_REPOSITORY https://github.com/hivcdm/cepac-transm.git
    GIT_TAG develop
    BUILD_COMMAND make
    INSTALL_COMMAND mv libcepac.a ${CMAKE_SOURCE_DIR}/lib/
)
ExternalProject_Get_Property(cepac_project source_dir)
include_directories(${source_dir}/src)
add_library(cepac STATIC IMPORTED)
set_property(TARGET cepac PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libcepac.a)
```

So at build time:
1. Cepac source is cloned from `develop` of `hivcdm/cepac-transm`.
2. Cepac is built into `libcepac.a` and dropped into `transm/lib/`.
3. Cepac's headers (`SimContext.h`, `Patient.h`, `RunStats.h`, etc.) are added to transm's include path.
4. transm links against the static archive.

Implication: **any change to cepac-transm `develop` will be picked up the next time transm is rebuilt clean**. There is no version pinning. If you need stability, push cepac changes onto a tagged branch and edit `GIT_TAG` accordingly.

## Configuration time — loading CEPAC `.in` files

The user provides `--cepac <directory>` on the command line, and the XML's `<cepacIntervention>` and `<artRolloutIntervention>` sections name `.in` filenames inside that directory. Each `.in` file becomes one `SimContext` object.

In [`Simulation::Run`](https://github.com/hivcdm/transm/blob/develop/src/core/simulation.cpp), the lambda `load_context`:

```cpp
auto load_context = [](const std::string &file_name) {
    auto context = new SimContext(file_name.substr(0, file_name.find(".in")));
    context->numPatientsToTrace = 0;     // disable CEPAC's per-patient trace
    context->readInputs();               // parse the .in file
    return context;
};
```

This is invoked for each `.in` file. Where the resulting `SimContext*` ends up depends on whether ART rollout is enabled:

- **Non-rollout** ([`simulation.cpp:986-991`](https://github.com/hivcdm/transm/blob/develop/src/core/simulation.cpp#L986)): pushed onto `EventParams::cepacSimContexts` (a `vector<SimContext*>`).
- **Rollout** ([`simulation.cpp:962-973`](https://github.com/hivcdm/transm/blob/develop/src/core/simulation.cpp#L962)): the default file's context goes into `EventParams::untreatedContext`. Each additional file is wrapped in a `RolloutContext(time, simContext, popOfInterest)` and pushed onto `EventParams::rolloutSimContexts`.

A separate path, [`EventParams::LoadCepacContext`](https://github.com/hivcdm/transm/blob/develop/src/parameters/eventparams.hpp#L222), is used by the targeted `<cepacContext>` intervention ([`simulationparametersxml.cpp:1700`](https://github.com/hivcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp#L1700)). It maintains a cache (`cepac_file_context_map_`) so the same file is loaded only once.

## Each Entity carries a `Patient`

[`Entity`](https://github.com/hivcdm/transm/blob/develop/src/entities/entity.hpp) holds a `Patient *cepacPatient`. When an entity becomes sexually active or becomes newly infected, transm:

1. Picks the appropriate `SimContext` via `getCEPACSimContextIndex()` (matches current sim time against `timesToSwitchSimContext`, plus rollout filters by population-of-interest)
2. Constructs a CEPAC `Patient` — passing the chosen SimContext, the global `RunStats` / `CostStats` / `Tracer`, and the entity's age-in-months and gender
3. Stores the `Patient*` on the entity

For an initially-infected entity, `applyPrevalentInfection` is the entry point. For someone newly infected mid-simulation, `becomeInfected` does the work.

## The monthly CEPAC step

Each month, for each living entity, transm calls into CEPAC to advance disease state. Inside [`Entity::updateHealthStatus`](https://github.com/hivcdm/transm/blob/develop/src/entities/entity.cpp):

```cpp
cepacPatient->simulateMonth();      // CEPAC runs its full month: BeginMonth → CD4/HVL update
                                    //   → ClinicVisit → ART efficacy → Mortality → EndMonth
```

After the call returns, transm reads the updated state back from CEPAC:

```cpp
cd4 = cepacPatient->getDiseaseState()->currTrueCD4;
hvl = HvlFromCepacHvl(cepacPatient->getDiseaseState()->currTrueHVLStrata);
```

This is the **only** way transm knows the patient's CD4 and HVL. The next month's transmission probability (which depends on infector HVL) reflects whatever CEPAC said.

## The HVL enum mapping problem

CEPAC and transm have **different HVL stratifications**. CEPAC uses `SimContext::HVL_STRATA` with 7 values (VLO through VHI). Transm uses `HVLStrata` with 10 values (UNINFECTED, ZERO through SIX, plus PRIMARY and LATESTAGE). [`HvlFromCepacHvl`](https://github.com/hivcdm/transm/blob/develop/src/entities/entity.cpp#L825) maps between them with a switch statement:

```cpp
case SimContext::HVL_VLO:  return HVLStrata::HVL_ZERO;
case SimContext::HVL__LO:  return HVLStrata::HVL_ONE;
case SimContext::HVL_MLO:  return HVLStrata::HVL_TWO;
// ...
case SimContext::HVL_VHI:  return HVLStrata::HVL_SIX;
default:                   throw std::runtime_error("invalid hvl");
```

`HVL_PRIMARY` and `HVL_LATESTAGE` are transm-only states (set directly when an entity is newly infected or in late-stage AIDS). They never come from CEPAC.

This mapping function is duplicated — manually — in several places in transm where switch statements convert CEPAC's enum back and forth. **This is the highest-risk part of the integration**: if either side adds an HVL stratum, all the switch statements need to be updated. The `TODO` calls for a `CepacPatientView` adapter to centralize these conversions.

## ART rollout: switching `SimContext`s

When an ART rollout time arrives, transm switches an entity's CEPAC `SimContext` from "untreated" to "treated" (or to a more specialized rollout context). [`Population::ApplyRolloutContext`](https://github.com/hivcdm/transm/blob/develop/src/core/population.cpp#L1523) walks through scheduled rollouts. For each rollout that fires, it:

1. Selects the right `RolloutContext` based on (current time, popOfInterest)
2. Reassigns `eventParams.untreatedContext` and/or `eventParams.treatedContext` to alias the new `SimContext` ([`population.cpp:1553-1651`](https://github.com/hivcdm/transm/blob/develop/src/core/population.cpp#L1553))
3. Calls `entity->setSimContext(newContext)` on every entity matching the rollout's population-of-interest, which delegates to `cepacPatient->setSimContext(...)` and resets the patient's clinical parameters

> **Bug history**: the v4.8 segfault at process exit was caused by `EventParams::~EventParams` trying to `delete` both `untreatedContext` and `treatedContext`, which after `ApplyRolloutContext` reassignments often pointed to the same `SimContext` (already owned by a `RolloutContext` in `rolloutSimContexts`). Fixed by making the destructor leak those aliased pointers — see the commit on `develop` after v4.8.0.

## Death propagation

CEPAC owns the mortality model. When CEPAC decides a patient is dead, transm reads it back during `updateHealthStatus`:

```cpp
if (!cepacPatient->isAlive()) {
    death = true;
    auto cause = cepacPatient->getDiseaseState()->causeOfDeath;
    // Map cepac's cause-of-death enum to transm's DeathStatus
}
```

Once `death == true`, the entity is removed from sexually-active buckets and won't appear in any future partnership draws. Its existing partnerships are dissolved via the `_fromDeath` flag in [`Male::getPartnershipsToEnd`](https://github.com/hivcdm/transm/blob/develop/src/entities/male.cpp#L658).

## Outputs

CEPAC writes its **own** output files (separate from transm's TSV traces):

```cpp
// simulation.cpp:594-606
if (parameters_.cepacRunStats->getPopulationSummary()->numCohorts > 0) {
    parameters_.cepacRunStats->finalizeStats();
    parameters_.cepacRunStats->writeStatsFile();
    parameters_.cepacCostStats->finalizeStats();
    parameters_.cepacCostStats->writeStatsFile();
}
```

These produce `cepacPopstats.out`, `cepacRunStats`, etc. — the files familiar from CEPAC standalone runs. They live alongside the transm trace files. Transm's `Population` and `Infections` traces are independent.

## When something looks wrong, where do you look first?

| Symptom | Likely culprit |
|---|---|
| Disease progression doesn't match expectation | The CEPAC `.in` file — open it in your editor, check ART regimens, mortality tables, etc. |
| Transmission probability is wrong | The `<transmissionCoefficients>` block in the XML, or the FOI formula in `Male::getFOI` / `Female::getFOI` |
| Mortality is wrong for HIV-positive entities | CEPAC mortality logic — debug from the CEPAC side, transm just reads `isAlive()` |
| Mortality is wrong for HIV-negative entities | The non-AIDS death table, parsed by [`CepacInputParser`](https://github.com/hivcdm/transm/blob/develop/src/utility/cepacinputparser.cpp) from the same CEPAC `.in` file (transm reads them, CEPAC doesn't, in this codepath) |
| Crash at end of simulation | Probably the v4.8 double-free, fixed on develop. If on a newer build, paste the gdb backtrace and dig from there |
| ART rollout times don't fire | `<targetRolloutProportions>` and the rollout time-switch logic in `Simulation::Run` |

## Why this boundary is a refactor target

The TODO file calls out the CEPAC boundary as the highest-leverage refactor opportunity. Right now, ~6 transm files reach into CEPAC internals directly (`getDiseaseState()->currTrueCD4`, etc.), and the HVL/CD4 enum mappings are duplicated as switch statements. A `CepacPatientView` adapter living in `src/utility/` would:

- Centralize all enum conversions in one place
- Let the rest of transm stop including CEPAC headers
- Make the model unit-testable without instantiating real `Patient`s
- Make swapping out CEPAC for a different disease engine plausible

This work has not been done. Tackling it would be a v5.0-worthy change.

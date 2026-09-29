# Architecture Overview

A 30,000-foot view of how the codebase is organized and how a simulation flows from input to output.

## Module layout

```
transm/
├── src/
│   ├── transm.cpp                  ← entry point (main + CLI)
│   ├── core/                       ← simulation lifecycle & population container
│   │   ├── simulation.cpp          ← Simulation::Run(), monthly time-step loop
│   │   ├── population.cpp          ← entity container, partnership orchestration,
│   │   │                              FOCUS screening, intervention dispatch
│   │   ├── intervention.hpp        ← Intervention class (callbacks at given times)
│   │   └── targetgroup.hpp         ← target groups for partition-style interventions
│   ├── entities/                   ← Entity, Male, Female, partnerships
│   │   ├── entity.cpp/.hpp         ← base Entity (huge — disease state, FOI helpers)
│   │   ├── male.cpp/.hpp           ← partnership-driving logic: rollNumPartners,
│   │   │                              rollNumEventsPerPartner, getFOI (M→F & MSM),
│   │   │                              ChoosePartnerDemographic
│   │   ├── female.cpp/.hpp         ← non-initiating; getFOI for F→M direction
│   │   ├── sexualpartnership.cpp   ← SexualPartnership struct & monthlySexualActivity
│   │   ├── sexualbehavior.cpp      ← SexualBehavior parameter bundle (Beta dist for
│   │   │                              condom, Poisson for acts, etc.)
│   │   └── prep.hpp                ← PrEP adherence/efficacy
│   ├── entitypool/                 ← bucketed indexing of entities for fast lookup
│   │   ├── entitypool.cpp          ← top-level pool
│   │   └── bucketsexualmixing.cpp  ← drawMember(), getRandomPerson() — partner draw
│   ├── parameters/                 ← input parsing + parameter bundles
│   │   ├── simulationparametersxml.cpp  ← the XML parser (~2050 lines)
│   │   ├── populationparameters.hpp     ← parameters that apply across the whole pop
│   │   ├── eventparams.hpp              ← live runtime state passed each event
│   │   └── parameterdefinitions.hpp     ← Calibration, RolloutContext, etc.
│   ├── statistics/                 ← every counter / output stream
│   │   ├── infectionstracker.cpp   ← incident & prevalent infections by HVL/age/etc.
│   │   ├── populationstatistics.cpp + populationstatisticsold.cpp
│   │   ├── coststracker.cpp        ← condom cost, circumcision cost, etc.
│   │   ├── artrollouttracker.cpp   ← ART rollout outputs
│   │   ├── preptracker.cpp         ← PrEP module outputs
│   │   └── tabularoutput.cpp       ← generic TSV writer
│   └── utility/                    ← randoms, time, paths, enum iteration, etc.
│       ├── randomnumbergenerator.cpp/.hpp
│       ├── time.cpp/.hpp
│       ├── cepacinputparser.cpp    ← reads NonAIDSDthProb tables from CEPAC .in
│       └── enum_iterator.hpp
├── doc/                            ← legacy docs (.docx files + Tutorial.md)
├── runs/                           ← canonical reference scenarios (test fixtures)
├── lib/                            ← static archives produced at build (libcepac.a etc.)
└── CMakeLists.txt                  ← build, including ExternalProject_Add for cepac
```

## Major classes at a glance

| Class | File | What it owns |
|---|---|---|
| `Simulation` | [`core/simulation.cpp`](https://github.com/hivcdm/transm/blob/develop/src/core/simulation.cpp) | Scenario lifecycle: load XML, set up CEPAC, run the monthly loop, emit outputs |
| `Population` | [`core/population.cpp`](https://github.com/hivcdm/transm/blob/develop/src/core/population.cpp) | All entities, partnership formation/dissolution, FOCUS screening, applies interventions |
| `Entity` / `Male` / `Female` | [`entities/`](https://github.com/hivcdm/transm/tree/develop/src/entities) | One simulated person; carries a CEPAC `Patient` |
| `SexualPartnership` | [`entities/sexualpartnership.cpp`](https://github.com/hivcdm/transm/blob/develop/src/entities/sexualpartnership.cpp) | A pair of entities + type, formation/dissolution time |
| `EventParams` | [`parameters/eventparams.hpp`](https://github.com/hivcdm/transm/blob/develop/src/parameters/eventparams.hpp) | Per-month state passed everywhere: clock, RNG, CEPAC contexts, trace file streams |
| `InfectionsTracker` | [`statistics/infectionstracker.cpp`](https://github.com/hivcdm/transm/blob/develop/src/statistics/infectionstracker.cpp) | All incident and prevalent infection counters |
| `BucketSexualMixing` | [`entitypool/bucketsexualmixing.cpp`](https://github.com/hivcdm/transm/blob/develop/src/entitypool/bucketsexualmixing.cpp) | Indexes entities by demographic profile + age for fast partner draws |

## End-to-end flow of a simulation

1. **Startup**: `main()` in [`transm.cpp`](https://github.com/hivcdm/transm/blob/develop/src/transm.cpp) parses CLI flags, calls `run_simulation(xml_path, cepac_dir, focus)`.
2. **XML parse**: [`SimulationParametersXml`](https://github.com/hivcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp) walks `focus_test.xml` and constructs `SimulationParameters` (population sizes, age distributions, transmission coefficients, partnership behavior, interventions, …).
3. **CEPAC contexts loaded**: [`Simulation::Run`](https://github.com/hivcdm/transm/blob/develop/src/core/simulation.cpp) instantiates one `SimContext` per CEPAC `.in` file referenced by the XML. Stored in `EventParams::cepacSimContexts` (non-rollout) or `EventParams::rolloutSimContexts` (rollout mode).
4. **Population built**: `Population` allocates `Entity` objects per the initial age/gender/risk distributions; each is given a CEPAC `Patient`.
5. **Monthly loop**: For each month from 0 to `<duration>` (in months):
   - Age every entity one month
   - Form new partnerships, dissolve expired ones — see [Partnership Formation](Mechanism-Partnership-Formation.md)
   - Process sexual activity in each existing partnership; draw transmissions — see [Transmission & FOI](Mechanism-Transmission-and-Force-of-Infection.md)
   - Step CEPAC for every entity (disease progression, ART, mortality)
   - Apply interventions whose time matches this month
   - Apply ART rollout context switches if scheduled
   - Apply FOCUS screening if `--focus` is enabled and we're in a target month
   - Update statistics; write trace-file rows for this month
6. **Final output**: After the last month, finalize CEPAC stats, write `summaryStats`, `IndividualSummaries.json`, etc.

## The transm ↔ CEPAC boundary

CEPAC is a sibling repository linked statically. Each `Entity` carries one `Patient *cepacPatient`; each month transm calls into CEPAC to advance disease state, then reads back CD4 and HVL to use in transmission probability. When ART rollouts fire, transm switches the `Patient`'s `SimContext` from "untreated" to "treated".

This boundary is detailed in [CEPAC Integration](Mechanism-CEPAC-Integration.md).

## Where computation actually lives

Roughly, a long-running simulation spends its time:

- **~50%** in partnership formation + sexual activity (entity loops, RNG draws, FOI evaluation)
- **~30%** in CEPAC's `Patient::simulateMonth()` — disease progression
- **~10%** in writing trace files (TSV row construction)
- **~10%** miscellaneous (population init, interventions, stats finalization)

If you're optimizing for throughput, those are the hot spots.

## Known structural pain points (from `TODO`)

- Entity / Male / Female should be merged into one polymorphic class with sexual-orientation-independent behavior modules. Currently MSM, MSMW, and heterosexual males are all `Male`-derived but switch on demographic profile internally, leading to a lot of `if(...isMsm)` branching.
- The CEPAC boundary is not encapsulated — direct access to `cepacPatient->getDiseaseState()->currTrueHVLStrata` appears across many files. A `CepacPatientView` adapter is overdue.
- `infectionstracker.hpp` has ~30 parallel counter members for slices of the same data; collapsing them into a single event log would simplify things significantly.

These don't block any current work but are worth knowing about.

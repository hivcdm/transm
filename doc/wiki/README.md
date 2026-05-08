# transm — HIV-CDM Transmission Model Wiki

`transm` is a stochastic, agent-based C++ simulation of sexual HIV transmission, coupled to the [CEPAC](https://www.massgeneral.org/medicine/mpec/research/cpac-model) disease-progression model. This wiki is the canonical reference for both the **science** of the model and the **code** that implements it.

> **Current release: [v4.8.0](https://github.com/hsphcdm/transm/releases/tag/v4.8.0)** — see the [CHANGELOG](https://github.com/hsphcdm/transm/blob/develop/CHANGELOG) for what shipped.

---

## Where to start

| If you are… | Start here |
|---|---|
| **A scientific user** running calibrations, comparing scenarios, or interpreting outputs | [Modeler's Guide](Modelers-Guide.md) |
| **A developer** joining the project, fixing bugs, or adding features | [Developer's Guide](Developers-Guide.md) |
| **Building / running** transm for the first time | [Build and Run](Build-and-Run.md) |
| Looking for **what the XML knobs do** | [Input XML Reference](Input-XML-Reference.md) |

---

## Deep dives — how the model actually works

Three mechanisms are responsible for almost everything `transm` does. Read these to understand the simulation rather than just the parameters.

- **[Partnership Formation](Mechanism-Partnership-Formation.md)** — how partners get selected each month, the male-driven loop, age/race assortativity, partnership duration and breakup.
- **[Transmission and Force of Infection](Mechanism-Transmission-and-Force-of-Infection.md)** — what happens during sex: the per-act probability, condoms, circumcision, microbicides, PrEP, and the dice roll that produces an infection.
- **[CEPAC Integration](Mechanism-CEPAC-Integration.md)** — how each entity carries a CEPAC `Patient`, how disease state (CD4, HVL) is read from CEPAC each month, and how ART rollout switches `SimContext`s.

---

## Reference

- [Architecture Overview](Architecture-Overview.md) — module layout, data flow, big classes
- [Input XML Reference](Input-XML-Reference.md) — every XML node and what it does
- [CEPAC Inputs Reference](CEPAC-Inputs-Reference.md) — `.in` files and how they're consumed
- [Outputs Reference](Outputs-Reference.md) — every TSV/output column and what counter produces it

---

## Reference inputs

The MIAMI calibration uses `focus_test.xml` as a representative configuration. CEPAC `.in` files live in `MIAMI-prime/INFILES/`. Sections of this wiki cite them by example.

---

## Conventions in this wiki

- File and line citations look like [`src/core/population.cpp:653`](https://github.com/hsphcdm/transm/blob/develop/src/core/population.cpp#L653) — clickable on GitHub when accessed via `develop`.
- "Per-month time step" means one iteration of the main simulation loop in [`src/core/simulation.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/core/simulation.cpp).
- "CEPAC" means the disease-progression model in the sister repo [`hsphcdm/cepac-transm`](https://github.com/hsphcdm/cepac-transm), linked statically as `libcepac.a`.
- When we say "Entity" we mean a simulated person — `Entity`, `Male`, or `Female` in [`src/entities/`](https://github.com/hsphcdm/transm/tree/develop/src/entities).

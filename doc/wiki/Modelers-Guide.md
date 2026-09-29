# Modeler's Guide

For scientific users running calibrations, comparing scenarios, and interpreting outputs. This guide is light on code and heavy on what the knobs mean.

## What `transm` actually models

`transm` is a **monthly-time-step, agent-based simulation** of HIV transmission via sexual contact. Every entity in the simulation is a discrete person with:

- Demographics (gender, sexual orientation, age, race, ethnicity, employment, risk group, relationship status)
- Sexual-behavior parameters drawn at birth (acquisition rates, condom-use chances, partnership durations, coital frequencies)
- An HIV status (negative, undiagnosed positive, diagnosed positive) and — if positive — a full CEPAC disease state

Each month, every living entity ages one month, may form or end partnerships, has sex within those partnerships, and (if HIV positive) has its disease state advanced one month by the CEPAC disease engine. The simulation runs for `<duration>` months (typically 1200 = 100 years).

Outputs are TSV files — one row per month — broken down by various demographic and clinical strata.

## Time

| Concept | Where it lives | Notes |
|---|---|---|
| Simulation month | `time_.in_months()` | 0 at start; advances by 1 each step |
| `monthOf1990` | `<monthOf1990>` in XML | Calibration anchor — the model's "month 600" maps to real-world Jan 1990 |
| `<duration>` | XML element | Total months to simulate, including burn-in |
| Burn-in | `<delay>` under `<initialInfections>` | Model runs without any infections until this month |

## Population structure

The simulation has **demographic profiles** that combine: gender × sexual orientation × relationship status × employment × risk group. Plus separate axes for race × ethnicity. The full product space is large; configurations enable a subset via `<allowedRaceAndEthnicities>` and the entity-type proportions.

```xml
<allowedRaceAndEthnicities>
  <allowedRace race="BLACK">
    <allowedEthnicity ethnicity="NON_HISPANIC"/>
  </allowedRace>
  <allowedRace race="WHITE">
    <allowedEthnicity ethnicity="HISPANIC"/>
    <allowedEthnicity ethnicity="NON_HISPANIC"/>
  </allowedRace>
</allowedRaceAndEthnicities>
```

A note on race × ethnicity: these are **orthogonal** in the data model. "Hispanic" is an ethnicity, not a race. Code that filters on `isWhite()` was historically careless about this and matched white Hispanics; v4.8 added `!isHispanic()` guards in the right places (see `FindWhiteMales*` in [`population.cpp`](https://github.com/hivcdm/transm/blob/develop/src/core/population.cpp)).

## Sexual behavior

Each male is born with a draw from population-level distributions for:

- **Acquisition rate** per partnership type (Steady, Regular, Casual, CSW) — a log-normal per (partnership type × risk level)
- **Coital events per month** per partnership type — a Poisson rate
- **Condom-use chance per event** per (risk × partnership type) — a Beta distribution
- **Partnership duration** — a shifted log-normal per (risk × type)
- **Average years younger** preferred for partners — a normal

Once drawn, these are **personal** to that male and persist through the simulation unless an intervention overrides them.

For details on how partnerships are formed each month from these baselines, see [Partnership Formation](Mechanism-Partnership-Formation.md).

## Transmission

Each coital event independently rolls for HIV transmission with probability:

```
FOI = base_transmission_for_HVL × (1 - condom_effect)
                                × (1 - microbicide_effect)    [F partner only]
                                × (1 - circumcision_effect)   [M susceptible only]
                                × (1 - prep_effect)
```

`base_transmission_for_HVL` is per-act, indexed by infector's viral load stratum, and direction (M→F, M→M, F→M). The matrix lives in the XML's `<transmissionCoefficients>`.

PrEP, condoms, microbicides, and (for the male susceptible) circumcision all stack multiplicatively. See [Transmission and Force of Infection](Mechanism-Transmission-and-Force-of-Infection.md) for the full formula and PrEP-adherence mechanics.

## Disease progression

Once an entity is HIV-positive, **CEPAC owns the disease**. Transm allocates a CEPAC `Patient` per positive entity, hands it a `SimContext` (one of the `.in` files referenced in the XML), and lets CEPAC's monthly clinical update advance CD4, HVL, OIs, ART status, and mortality. Each month transm reads back CD4 and HVL and uses them in the next month's transmission risk.

If you want to change how the disease progresses, edit the CEPAC `.in` file or modify CEPAC itself. See [CEPAC Integration](Mechanism-CEPAC-Integration.md).

## Interventions

The XML can configure many interventions, applied at scheduled simulation times:

- **ART rollout** — switches subgroups of HIV-positive entities from the "untreated" to a "treated" CEPAC `SimContext`. Configured by `<artRolloutIntervention>` and `<targetRolloutProportions>`.
- **PrEP** — gives entities an efficacy multiplier and an adherence level. See `<preExposureProphylaxisUse>` interventions.
- **Vaginal microbicide** — analogous to PrEP but for the female partner.
- **Circumcision** — sets `circumcised = true` on a male, providing per-act protection.
- **Condom-use overrides** — force a specific condom-use probability for targeted entities.
- **FOCUS screening** — race- and sex-stratified focused testing of undiagnosed and lost-to-follow-up populations. Enabled with `--focus` on the command line. New in v4.8.
- **Per-parameter interventions** — almost any sexual-behavior parameter can be modified at a specific time via `<generalInterventions>`.

## Race-specific multipliers (v4.8 status)

Three race-specific knobs currently exist:

| Knob | Type | Configurable via XML? | Where |
|---|---|---|---|
| Coital acts × race for Steady/Regular partnerships | hardcoded multipliers (5×, 0.33×, 1.33×) | **No** — hardcoded in [`male.cpp:524-541`](https://github.com/hivcdm/transm/blob/develop/src/entities/male.cpp#L524) | Issue #91 — TODO to parameterize |
| Condom-use × race | parameterized | **Yes** — `<multiplierCondomUseBlacks>` and `<multiplierCondomUseWhites>` | Issue #89 |
| Race-ethnic assortativity | parameterized | **Yes** — `<assortivity>` block | Existing |

### Race-specific multipliers leak across races

This is non-obvious enough to warrant calling out. Because the partner-race draw is sometimes non-assortative (with probability driven by `getRaceEthnicAssortativeness`), the partnership network mixes across racial groups. So a behavioral multiplier targeted at "Black males' coital acts" applies to **every female partner of a Black male**, regardless of her race. Effects propagate through the network into other groups.

This is a feature of the model (it captures real-world cross-racial mixing) but it's surprising the first time you see it. If you want a race-specific multiplier to affect *only* same-race partnerships, you'd need to condition on **both** partners' race in the affected code path. Issue #91 has more discussion.

## Calibration anchor: `monthOf1990`

The model's internal time origin is "month 0" but real-world dates are anchored by `<monthOf1990>` (typically 600). Outputs that need calendar years (the `ShiftedOutcomes` trace, for example) shift by this offset. Make sure your XML's `<monthOf1990>` matches your CEPAC `.in` file's calibration.

## Reading the outputs

A typical run produces ~10 trace files. The most important ones:

- **Population.xls** — population size, age structure, demographic counts month by month
- **Infections.xls** — incident and prevalent infections by HVL, age, gender, etc.
- **Partnership.xls** — partnership formation, dissolution, concurrency patterns
- **CalibStats.xls** — summary numbers for matching calibration targets
- **ARTRollout.xls** — ART access, treatment, and outcome counts
- **PrepOutcomes.xls** — PrEP module outputs
- **cepacPopstats** — CEPAC's standard population-summary output (separate from transm's stats)
- **summaryStats** — a single-line summary of the run

See [Outputs Reference](Outputs-Reference.md) for column-by-column meaning.

## Reproducibility

- Set `<fixedSeed>` to a positive integer for deterministic runs. `0` uses a default fixed seed; `-1` uses system time.
- Different transm versions can produce different results from the same seed (RNG-call order has shifted across releases — e.g. an extra random draw earlier in the loop reshuffles all subsequent draws). Pin to a tag for cross-run reproducibility.

## Common mistakes

1. **Comparing flow vs stock.** "New Infections" is monthly flow; many other counters are prevalent stock. They will not equal each other except in a perfectly stationary epidemic.
2. **Assuming `isWhite()` excludes Hispanics**. It does now, in the FOCUS find functions and the white coital-acts multiplier (post-v4.8 fix). But if you write new code that filters on race, double-check.
3. **Editing CEPAC `.in` files for transmission parameters.** Transmission probabilities live in the **transm XML**, not the CEPAC `.in`. The CEPAC file is for disease progression, not transmission.
4. **Forgetting to pull cepac.** The build re-clones cepac from `develop` on every clean configure. If you fix a bug in cepac-transm, push it to that branch before re-building transm.

# Mechanism: Transmission and Force of Infection

How a coital event becomes (or doesn't become) an HIV transmission. This is the scientific core of the model.

## Per-month entry

[`SexualPartnership::monthlySexualActivity`](https://github.com/hivcdm/transm/blob/develop/src/entities/sexualpartnership.cpp#L110) is called once per partnership per month, by the male partner (the initiator).

```cpp
int eventsThisMonth = partners[0]->rollNumEventsPerPartner(partners[1], rng, type);
if (eventsThisMonth <= 0) eventsThisMonth = 1;       // floor at 1
return partners[0]->sexualActivity(partners[1], eventsThisMonth, type, ...);
```

So the flow is: **decide how many acts → run them in a loop → return whoever got infected (if anyone)**.

## Step 1 — number of coital acts

[`Male::rollNumEventsPerPartner`](https://github.com/hivcdm/transm/blob/develop/src/entities/male.cpp#L519) draws from a Poisson distribution with these modifications:

```cpp
double meanCoitalEvents = numActsPerMonth[type];

// Race-specific calibration multipliers (MIAMI), Steady & Regular only
if (isBlack()    && (type == Steady || type == Regular)) meanCoitalEvents *= 5.0;
if (isWhite() && !isHispanic() && (type == Steady || type == Regular)) meanCoitalEvents *= 0.33;
if (isHispanic() && (type == Steady || type == Regular)) meanCoitalEvents *= 1.33;

// Older men have fewer acts
if (ageYrs >= partneringDiscStartAgeYrs) meanCoitalEvents *= getPartneringActsDiscMult(ageYrs);

if (meanCoitalEvents < 1) meanCoitalEvents = 1;             // floor at 1
return randPoisson(meanCoitalEvents - 1) + 1;               // ensures ≥1 acts
```

These multipliers are currently **hardcoded** with a TODO to expose them as XML parameters. See the v4.8 release notes.

## Step 2 — concordance check

Inside [`Entity::sexualActivity`](https://github.com/hivcdm/transm/blob/develop/src/entities/entity.cpp#L721), the very first thing checked:

```cpp
if (isInfected() == _p->isInfected()) {
    return nullptr;     // both positive or both negative — nothing to transmit
}
```

So discordant pairs are the only ones that go further.

## Step 3 — the per-act loop

For each act in `1..eventsThisMonth`:

```cpp
double foifPerEvent = infected->getFOI(uninfected, transmission_coefficients, type, eventParams);
bool condomUsed = infected->getCondomUsedLastFOICalculation();
if (condomUsed) {
    incrementCondomsUsedThisMonth(1);
    _p->incrementCondomsUsedThisMonth(1);
}
infTrack->recordExposure(currTime, infected, type, condomUsed);   // every act, transmitted or not

if (rng.chance(foifPerEvent)) transmissionOccured = true;
```

A few things to notice:

- **Exposure is recorded for every act**, even ones that don't transmit. This drives the "exposures" columns in the output.
- **Condom use is decided once per call to `getFOI`** (not once per partnership-month). Each act independently rolls for condom use.
- **The transmission die is rolled per-act**. If any act transmits, `transmissionOccured` flips and the loop continues to its end. Multiple acts in the same partnership-month can't infect the same partner twice, but they each contribute exposures.

After the loop, if `transmissionOccured`:

```cpp
uninfected->becomeInfected(infected->getGenerationOfInfection(false) + 1, eventParams);
return uninfected;       // signals the caller that this partner just got infected
```

## Step 4 — Force of Infection (the meat)

`getFOI` returns the per-act transmission probability. The formulas for the two directions differ slightly.

### Male-to-female (and male-to-male)

[`Male::getFOI`](https://github.com/hivcdm/transm/blob/develop/src/entities/male.cpp#L345):

```cpp
double chanceCondomUse = getCondomUseProb(_p, type);
if (_p->HasOverrideChanceCondomUse())
    chanceCondomUse = _p->GetOverrideChanceCondomUse();
condomUsedLastFOICalculation = rng.chance(chanceCondomUse);

double condomEff = condomUsedLastFOICalculation ? getCondomProtectEff() : 0;

double baseFoi, microbicideEfficacy = 0;
if (_p is Female) {
    baseFoi = transmission_coefficients[male_to_female][getHVL()];   // infector's HVL!
    if (Female->RollForVaginalMicrobicideUse(rng))
        microbicideEfficacy = Female->GetVaginalMicrobicideEfficacy();
} else {
    baseFoi = transmission_coefficients[male_to_male][getHVL()];
}

double prepEfficacy = _p->GetPreExposureProphylaxisEfficacy();
switch (_p->getAdhereceStatus()) {
    case PREP_SUBSTANTIALLY_ADHERENT: prepEfficacy *= 0.80; break;
    case PREP_PARTIALLY_ADHERENT:     prepEfficacy *= 0.10; break;
    case PREP_INADHERENT:             prepEfficacy *= 0.05; break;
    case PREP_ADHERENT:               prepEfficacy *= 0.99; break;
    case PREP_OFF_PREP:               prepEfficacy *= 0.0;  break;
}

return baseFoi * (1 - condomEff) * (1 - microbicideEfficacy) * (1 - prepEfficacy);
```

Note for M→F: **circumcision does NOT apply in this direction**. The base `transmission_coefficients[male_to_female]` is meant to represent the average risk regardless of male circumcision status; the protection comes in only when the male is the *receiver* (M↔M or F→M).

### Female-to-male

[`Female::getFOI`](https://github.com/hivcdm/transm/blob/develop/src/entities/female.cpp#L155):

```cpp
double condomUseProb = _p->getCondomUseProb(this, type);    // read from MALE partner
condomUsedLastFOICalculation = rng.chance(condomUseProb);

double condomEff = condomUsedLastFOICalculation ? _p->getCondomProtectEff() : 0;
double circEff   = _p->getCircumProtectEff();               // applies in F→M direction

double baseFoi = transmission_coefficients[female_to_male][getHVL()];

// PrEP on the (male) susceptible
double prepEfficacy = _p->GetPreExposureProphylaxisEfficacy();
// ... same adherence multipliers ...

return baseFoi * (1 - condomEff) * (1 - circEff) * (1 - prepEfficacy);
```

Two important asymmetries:
- The female reads condom-use probability from her **male** partner — males own the condom-use distribution.
- Circumcision protection (`circEff`) applies only in this direction, because the susceptible is the (potentially circumcised) male.

### Composite formula in plain English

```
FOI(per act) = base_transmission_for_HVL × (1 - condom_effect)
                                          × (1 - microbicide_effect)   [F partner only]
                                          × (1 - circumcision_effect)  [M susceptible only]
                                          × (1 - prep_effect)
```

Each protective mechanism is a multiplicative reducer; they don't sum.

## Step 5 — what `becomeInfected` does

When transmission fires, [`Entity::becomeInfected`](https://github.com/hivcdm/transm/blob/develop/src/entities/entity.cpp#L266):

- Records `timeOfInfection` and `ageAtInfection`
- Sets `generationOfInfection = infector_gen + 1`
- Sets `hvl = HVLStrata::HVL_PRIMARY` (initial post-acquisition stage)
- Initializes / configures the entity's CEPAC `Patient` so disease progression starts (this is where a lot of CEPAC-side state gets set up for this brand-new positive)

The new state is durable — the entity carries it into all future months.

## Step 6 — recording the infection

After `monthlySexualActivity` returns, the caller in [`Entity::allPartnerSexualActivity`](https://github.com/hivcdm/transm/blob/develop/src/entities/entity.cpp#L196) appends the newly infected entity to `_newlyInfected`. Then in [`Population::UpdatePartnerships`](https://github.com/hivcdm/transm/blob/develop/src/core/population.cpp#L653):

```cpp
populationStatistics.recordIncidentInfection(eventParams, currTime, type, wasInfected, wasUninfected);
```

This walks through:

1. `PopulationStatisticsOld::recordIncidentInfection` — bumps survival stats, yearly counters
2. `InfectionsTracker::recordIncidentInfection` — bumps every per-(HVL × age × gender × risk × employment × partnership-type) incident counter

There used to be a few bugs in this last layer (parameter mix-up between infector and infected, an uninitialized `incidentInfsGender` array, and a `<isWhite>` filter that matched white-Hispanics). All fixed in v4.8 — see the [`infectionstracker.cpp` history](https://github.com/hivcdm/transm/commits/develop/src/statistics/infectionstracker.cpp) for the gory detail.

## Sanity-check invariants

By construction (after the v4.8 fixes):

- `New Infections` (column 2 of the `Infections` TSV) **==** `Male incident + Female incident` for every row.
- `Total Infected in History` (column 3) **==** sum of cumulative-by-gender Male+Female columns at line 1086+ of the same file.
- Sum over age buckets of `Male (By Age)` + `Female (By Age)` equals total `New Infections` — provided every infected entity's age fits into one of the configured age ranges.

If any of these invariants break, something is wrong with the counting; file an issue.

## Where to look when you want to change something

| Change | File:line |
|---|---|
| Tune base transmission probabilities by HVL | XML `<transmissionCoefficients>` → parsed in [`simulationparametersxml.cpp`](https://github.com/hivcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp) |
| Adjust circumcision efficacy | `<circumProtectEff>` → applied in `Female::getFOI` |
| Adjust condom efficacy | `<condomProtectEff>` → applied in both directions |
| Adjust PrEP adherence multipliers | currently **hardcoded** at [`male.cpp:389-401`](https://github.com/hivcdm/transm/blob/develop/src/entities/male.cpp#L389) and `female.cpp:184-197` — TODO to parameterize |
| Race-specific coital act multipliers | currently **hardcoded** at [`male.cpp:524-541`](https://github.com/hivcdm/transm/blob/develop/src/entities/male.cpp#L524) — TODO to parameterize (see Issue #91) |
| Race-specific condom-use multipliers | live, parameterized via `<multiplierCondomUseBlacks>` / `<multiplierCondomUseWhites>` (Issue #89) |

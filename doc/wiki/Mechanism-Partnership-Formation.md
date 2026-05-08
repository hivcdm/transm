# Mechanism: Partnership Formation

How partners are selected and partnerships are formed each month. This is the most complex single mechanism in the model — partnerships drive everything downstream (sexual activity, transmission, demographic mixing).

## High-level summary

Each simulated month, for each living male:

1. **Dissolve** any existing partnerships that have hit their end (duration expired, breakup-rate dice roll, or partner died).
2. **Form** new partnerships — for each partnership type (Steady, Regular, Casual, CSW), draw a Poisson number of new partners and select each one from a demographic bucket.
3. **Have sex** in every active partnership (new and existing). The male partner drives this; the female does not loop over her partnerships from her side.

Females never initiate. They get drawn into partnerships by males and then participate when sexual activity is triggered. There is no explicit cap on simultaneous partnerships, but `concurrencyDef` records the patterns for calibration.

## Entry point — the monthly time-step

The partnership work happens inside the call chain:

[`Simulation::Step()`](https://github.com/hsphcdm/transm/blob/develop/src/core/simulation.cpp#L458) → `SimulateMonth()` → [`Population::UpdatePartnerships(eventParams)`](https://github.com/hsphcdm/transm/blob/develop/src/core/population.cpp#L521).

Inside `UpdatePartnerships`, two passes over the male population:

```cpp
// Pass 1 — dissolve ended partnerships
for (each Male) {
    for (each partnership type) {
        getPartnershipsToEnd(...);    // src/entities/male.cpp:626
    }
    DissolveSexualPartnerships(...);  // src/core/population.cpp:1112
}

// Pass 2 — form new ones, then have sex with everyone (old + new)
for (each Male) {
    for (each partnership type) {
        CreatePartnerships(...);      // src/core/population.cpp:2379
    }
    for (each partnership type) {
        allPartnerSexualActivity(...);// src/entities/entity.cpp:176
    }
}
```

## Who initiates? Males do.

Both passes loop over the male side of the entity pool only. Female entities are never the outer subject of a partnership-forming or sex-initiating loop. This is enforced inside [`Entity::allPartnerSexualActivity`](https://github.com/hsphcdm/transm/blob/develop/src/entities/entity.cpp#L191) by the check `if ((*iter)->getPartner1() == this)` — only `partner1` (the male in MSW partnerships, the designated initiator in MSM partnerships) initiates.

A male is gated out of partnership formation if any of these are true ([`population.cpp:588-590`](https://github.com/hsphcdm/transm/blob/develop/src/core/population.cpp#L588)):

- He's not yet at `popWideParams.ageOfMajority + GetSexualActivityDelay()`
- His sexual-activity status is `NotActive`
- He's a CSW (only non-CSW males are partnership initiators)
- He's dead

## Number of new partners — the Poisson draw

For each (male, partnership type), the number of new partners this month is drawn by [`Male::rollForNumPartners`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L527). The formula:

```cpp
double partnerRate = partnerAcqRates[type];        // baseline, set at init from a log-normal

// MSM correction: MSM males have inflated rates; halve to avoid double-counting,
// then apply the 1.61 behavioral multiplier
if (orientation == Msm)
    partnerRate *= 0.5 * 1.61;

// If we already have a steady partner and we're rolling for a non-Steady type,
// dampen acquisition
if (haveSteady)
    partnerRate *= getPartnerAcqMultWithSteady(riskLevel);

// Older men acquire partners less often
if (ageYrs >= partneringDiscStartAgeYrs)
    partnerRate *= getPartneringAcqDiscMult(ageYrs);

int numPartners = randPoisson(partnerRate);
return (type == Steady) ? std::min(1, numPartners) : numPartners;
```

The Steady type is clamped to `≤ 1` because each male can only have one steady partner at a time.

The baseline `partnerAcqRates[type]` is set per-male at construction by drawing from the population-level log-normal `acquisition_rate_dist` ([`male.cpp:301-308`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L301)). It does not change during the simulation unless an intervention rewrites it.

## Choosing a specific partner

Once `numPartners` is drawn, the male picks each partner via [`Male::ChoosePartnerDemographic`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L182). This builds a demographic profile filter (gender × orientation × CSW × relationship status × race × ethnicity) and resolves to a single `BucketSexualMixing` from which a random eligible person is drawn.

### Step 1 — partner CSW status

- `_partnershipType == Csw` → CSW partner, single ([`male.cpp:195-199`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L195))
- Otherwise → non-CSW partner; relationship status is decided probabilistically via `behavior.getChanceChooseWithSteady()`

### Step 2 — partner gender + orientation

- **Heterosexual male** → female MSW partner
- **MSM** → male partner (probabilistically MSM vs MSMW based on `getChanceMsmChooseMsmw()`)
- **MSMW** → with `getChanceMsmwChooseMale()` chooses male partner; otherwise female

### Step 3 — partner age

`Male::rollForAgeDifference` draws from a normal distribution (mean = `averageYearsYounger[type]`):

```cpp
double ageYoungerYears = _randomNums.randNorm(averageYearsYounger[type]);
// Partner age range: [chooserAge - ageYoungerYears - 0.5y, chooserAge - ageYoungerYears + 0.5y]
```

The 1-year window (±6 months) is a numerical convenience to keep the candidate pool nonempty.

### Step 4 — partner race / ethnicity (assortativity)

This is one of the more interesting decisions. With probability [`getRaceEthnicAssortativeness(entityRace, entityEthnicity)`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L240):

- **Assortative**: partner has same race AND ethnicity as the chooser (homogeneous mixing).
- **Non-assortative**: race is sampled uniformly from `allowedRaceEthnicityMap.GetRaceKeysFromMap()`, then ethnicity uniformly from that race's allowed list.

This is what makes "shifting coital acts for whites also affects hispanics" — non-assortative draws cross racial lines, so behavioral changes in one group leak into others through the partnership network. See the model-level discussion in the [Modeler's Guide](Modelers-Guide.md).

### Step 5 — drawing the actual person

Once the demographic profile is fully specified, [`BucketSexualMixing::drawMember`](https://github.com/hsphcdm/transm/blob/develop/src/entitypool/bucketsexualmixing.cpp) takes the age-range constraints and returns a uniformly random person from the eligible pool — optionally removing them so they can't be selected again in this month for this male (`_remove` flag).

## Building the partnership

When a partner is chosen and accepted, a [`SexualPartnership`](https://github.com/hsphcdm/transm/blob/develop/src/entities/sexualpartnership.cpp#L20) is constructed. Its key fields:

- `partners[0]` = the male initiator (always partner1)
- `partners[1]` = the chosen partner
- `type` = Steady / Regular / Casual / CSW (or the MSM variants)
- `timePartnerFormation` = current month
- `timePartnerDissolution` = formation + duration

**Duration** is drawn by [`Male::rollForNewPartnershipDuration`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L616) from a *shifted* log-normal distribution (`randShiftedLogNormal`), per (risk × type). For partnerships formed at `time == 0` (initial population), the duration is randomly truncated so they don't all dissolve in lockstep.

The new partnership is pushed into both partners' `partners[type]` list ([`entity.hpp:156`](https://github.com/hsphcdm/transm/blob/develop/src/entities/entity.hpp#L156)). If a Steady partnership was just added, the male's `RelationshipStatus` flips from `Single` to `NonSingle` (and the bucket index is updated accordingly).

## Sexual activity in existing partnerships

After Pass 2 finishes forming new partnerships, [`Entity::allPartnerSexualActivity`](https://github.com/hsphcdm/transm/blob/develop/src/entities/entity.cpp#L176) iterates all partnerships of the given type belonging to this male:

```cpp
for (auto partnership : partners[type]) {
    if (partnership->getPartner1() == this) {     // only one side initiates
        Entity *infected = partnership->monthlySexualActivity(...);
        if (infected) _newlyInfected.push_back(infected);
    }
}
```

Each `monthlySexualActivity` ([`sexualpartnership.cpp:110`](https://github.com/hsphcdm/transm/blob/develop/src/entities/sexualpartnership.cpp#L110)) calls `Male::rollNumEventsPerPartner` for the count of acts, then loops `_numActs` times calling `getFOI` per act. That's the [Transmission and Force of Infection](Mechanism-Transmission-and-Force-of-Infection.md) flow.

## Partnership ending

[`Male::getPartnershipsToEnd`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L626) decides each month whether each existing partnership should dissolve:

| Type | MSW breakup rate | MSM/MSMW breakup rate |
|---|---|---|
| Steady | `getBreakupRateMSW(Steady)` | `0.5 * getBreakupRateMSM(Steady)` |
| Regular | `getBreakupRateMSW(Regular)` | `0.5 * getBreakupRateMSM(Regular)` |
| Casual | `1.0` (always end after one month) | `0.5 * 1.0` |
| CSW | always end after one month | n/a |

The `0.5` on MSM rates corrects for the fact that both MSM partners would otherwise each get a chance to end the partnership.

Death also dissolves all of an entity's partnerships immediately ([`male.cpp:658`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L658) `_fromDeath` flag).

When a partnership ends, [`DissolveSexualPartnerships`](https://github.com/hsphcdm/transm/blob/develop/src/core/population.cpp#L1112) removes it from both partners' `partners[type]` lists, deletes the `SexualPartnership`, and updates relationship-status buckets if a Steady partnership ended.

## Concurrency tracking

There is **no enforced cap** on the number of simultaneous partnerships of any kind. However, [`concurrencyDef`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/eventparams.hpp#L159) defines up to 256 patterns (8-bit mask over the 8 partnership types) that are *recorded* whenever they occur. This data is exported for calibration to empirical concurrency targets but does not affect the simulation dynamics.

## Pitfalls and edge cases

1. **Partnership formation order matters within a month.** If Male M1 and Male M2 both target the same Female F via the bucket draw, the first one to be processed in the outer loop gets her (with `_remove = true` she's removed from the pool). Outer-loop order is iteration order of the male bucket, which is deterministic given a fixed seed.

2. **Self-partnerships are blocked** in MSM partnerships by an explicit check on the chooser identity in `BucketSexualMixing::drawMember`.

3. **Females never initiate.** This means a partnership type that exists only "from the female side" is impossible to express in the current model.

4. **Race-based behavioral interventions leak across races.** Because of the non-assortative branch in step 4 (race/ethnicity assortativity), a multiplier you apply to "white males' coital acts" affects every female partner of those white males, regardless of her race. See the v4.8 calibration discussion in the [Modeler's Guide](Modelers-Guide.md#race-specific-multipliers).

5. **Concurrency is recorded but not enforced.** If a male's partnerships imply a concurrency pattern your model assumed wouldn't occur, the simulation runs anyway — you'll only see it in the output.

# Input XML Reference

A block-by-block guide to the XML scenario file. For each block we cover three things:

1. **The XML** — what the block looks like, with a snippet from `focus_test.xml`
2. **What it controls** — what the modeler is configuring
3. **Inner mechanism** — what the simulation actually does with the values

If you're new to running models, read straight through. If you're looking up a specific tag, use the table of contents in the sidebar (or your browser's find).

> Reference scenario: `MIAMI-prime/focus_test.xml`. Copy and modify rather than starting from scratch. Every snippet below is taken from that file.

---

## Top-level structure

```xml
<simulation version="4.5">
    <debugLevel>0</debugLevel>
    <duration>1000</duration>          <!-- months -->
    <fixedSeed>0</fixedSeed>
    <monthOf1990>600</monthOf1990>

    <traceFiles> ... </traceFiles>
    <concurrencyDefinition> ... </concurrencyDefinition>
    <calibration enabled="false"> ... </calibration>
    <population id="Miami"> ... </population>
    <interventions> ... </interventions>
</simulation>
```

The `version=` attribute is informational; it does **not** gate parser behavior.

---

## Simulation-level controls

### `<debugLevel>`

Integer (0–N). Higher values print more diagnostic output to stdout. `0` is normal.

### `<duration>`

Total number of months to simulate, including burn-in. `1000` means 83.3 years. For MIAMI scenarios that anchor on calendar 1990 at month 600, this means the run covers ~1942 through ~2025.

### `<fixedSeed>`

| Value | Meaning |
|---|---|
| `-1` | Seed the RNG from system time — non-deterministic, run-to-run different |
| `0` | Use a default fixed seed (currently `36291434`) — deterministic, reproducible |
| `>0` | Use this specific positive integer as the seed |

**Inner mechanism**: A single `RandomNumberGenerator` instance (mt19937 wrapper) lives in `EventParams::randomNums` and is used everywhere. Adding a single extra random draw anywhere in the code shifts the seed for *all* subsequent draws — so even with a fixed seed, identical results across versions are not guaranteed.

### `<monthOf1990>`

The simulation's month that maps to real-world January 1990. Typical value: `600` (so month 0 = Jan 1940, month 1200 = Jan 2040). Used to:

- Anchor the calibration year ranges in `<yearlyIncidenceRanges>` to real years
- Compute calendar dates for shifted-outcomes output
- Cost discounting alignment with the CEPAC `.in` file

Make this consistent across your XML and your CEPAC `.in` files or discounting will misalign.

---

## `<traceFiles>` — what gets written out

```xml
<traceFiles>
    <population enabled="true" tossIfCalibFail="false">
        <extension>Population.xls</extension>
    </population>
    <infection enabled="true" tossIfCalibFail="false">
        <extension>Infections.xls</extension>
    </infection>
    <!-- ... -->
    <costEffectiveness enabled="false" tossIfCalibFail="true">
        <extension>CE.xls</extension>
        <condomCost>0.5</condomCost>
        <circumcisionCost>500</circumcisionCost>
        <prEPCost>1</prEPCost>
        <vaginalMicrobicideCost>1</vaginalMicrobicideCost>
    </costEffectiveness>
    <singlePerson enabled="false" tossIfCalibFail="true">
        <extension>SinglePerson.xls</extension>
        <numberToTracePerAgeRange>15</numberToTracePerAgeRange>
        <numberNewbornsToTrace>500</numberNewbornsToTrace>
        <monthTraceNewborns>600</monthTraceNewborns>
        <tracePrevalentCases>1</tracePrevalentCases>
    </singlePerson>
</traceFiles>
```

**What it controls**: which output files the simulation writes, what file extension to use, and whether to delete the file if calibration fails.

**Inner mechanism**: Each `TraceFile::Type` (`Population`, `Infection`, `Partnership`, `Survival`, `CostEffectiveness`, `Clinical`, `Events`, `SinglePerson`, `LifeExpectancy`, `PartnerAcquisition`, `PartnerNetwork`, `CalibrationStatistics`, `ArtRollout`, `PrepOutcomes`, `ShiftedOutcomes`) gets an `std::ofstream` in `EventParams::trace_files`. When the corresponding `Population::Print*` method runs each month, it streams a row into that file. Disabling a trace just means the stream is never opened and the print methods skip it.

`tossIfCalibFail="true"` means: if the run fails partnership-formation calibration at the end (see `<calibration>`), delete this trace file. Useful for keeping only the runs you care about during a sweep.

**Special elements**:
- `<costEffectiveness>` carries its own cost parameters (condom, circumcision, PrEP, microbicide). These are only used if CE is enabled.
- `<singlePerson>` controls per-individual tracing:
  - `<numberToTracePerAgeRange>` — pick this many entities at sim start per age bucket and follow them for life
  - `<numberNewbornsToTrace>` / `<monthTraceNewborns>` — start tracing newborns at this calendar month
  - `<tracePrevalentCases>` — 1 to include initial prevalent infections in the trace
- `<partnerAcquisition>` and `<lifeExpectancy>` take `<time>` children — specific months at which a snapshot is recorded.

See [Outputs Reference](Outputs-Reference.md) for what each trace file actually contains.

---

## `<concurrencyDefinition>` — patterns of overlapping partnerships

```xml
<concurrencyDefinition>
    <definition id="0">
        <minNeeded>2</minNeeded>
        <allow>1</allow>
    </definition>
    <!-- ... up to 256 definitions ... -->
</concurrencyDefinition>
```

**What it controls**: patterns of simultaneous partnerships that should be *counted* in the output (typically for calibration to empirical concurrency data).

**Inner mechanism**: There is **no enforced cap** on the number of simultaneous partnerships an entity can have. This block declares which patterns we care about. Each `<definition>` is a bitmask (8 bits for 8 partnership types) checking whether an entity has enough partnerships of certain types simultaneously. Up to `256` definitions can be configured. When an entity matches, `setTimeOfLatestConcurrent()` is called for downstream reporting. Used to match against real-world concurrency surveys during calibration.

If you're not calibrating to concurrency, you can leave this minimal (one default definition) and ignore it.

---

## `<calibration>` — pass/fail gates for sweep runs

```xml
<calibration enabled="false">
    <monthOfCalibration>660</monthOfCalibration>
    <partnershipOutcomes>
        <steadyPrev>
            <popOfInterest>0</popOfInterest>
            <lwrBound>0.35</lwrBound>
            <uprBound>0.50</uprBound>
        </steadyPrev>
        <casualPrev> ... </casualPrev>
        <numActs> ... </numActs>
        <!-- etc. -->
    </partnershipOutcomes>
    <yearlyIncidenceRanges>
        <yearlyIncidenceRange time="648" lower="0.0001518" upper="0.0013660"/>
        <!-- ... -->
    </yearlyIncidenceRanges>
</calibration>
```

**What it controls**: pass/fail thresholds for whether a parameter sweep run "succeeds". When `enabled="true"`, at the end of the simulation each metric is checked against its bounds. If any metric is out of range, the run is marked failed and trace files with `tossIfCalibFail="true"` get deleted.

**Inner mechanism**: `passedCalibration_` boolean is set during `Simulation::Run()`. If false at end, the trace-file disposal loop in `Simulation::Run` at the end of the run deletes the marked traces (`simulation.cpp:608-618`).

**Common metrics**:
- `<steadyPrev>` / `<casualPrev>` / `<cswPrev>` — proportion of population with at least one partnership of that type
- `<numActs>` — average coital acts per month (per partnership)
- `<propInCon>` — proportion of the population in concurrent partnerships
- `<yearlyIncidenceRange>` — HIV incidence at specific simulation months should be within `[lower, upper]`

If you're not doing a sweep, leave `enabled="false"`.

---

## `<population>` — the big block

This is the largest section and configures everything about the population: size, age structure, demographics, transmission probabilities, PrEP, per-entity-type behavior.

```xml
<population id="Miami">
    <ageOfMajority>15</ageOfMajority>
    <initialState useCounts="false"> ... </initialState>
    <births useBirthRate="false"> ... </births>
    <initialInfections> ... </initialInfections>
    <transmissionCoefficients> ... </transmissionCoefficients>
    <preExposureProphylaxis> ... </preExposureProphylaxis>
    <entities>
        <entity type="male"> ... </entity>
        <entity type="female"> ... </entity>
        <entity type="msm"> ... </entity>
        <entity type="msmw"> ... </entity>
    </entities>
</population>
```

### `<ageOfMajority>`

Age in years at which an entity can become sexually active. Below this, the entity is in the population but won't form partnerships. Typically `15`.

### `<initialState>` — size and demographics at month 0

```xml
<initialState useCounts="false">
    <size>1000000</size>
    <useCounts>false</useCounts>
    <entityDistributions>
        <ageRangeDistributions>
            <ageRange lower="0"  upper="14">0.15</ageRange>
            <ageRange lower="15" upper="19">0.07</ageRange>
            <!-- ... -->
            <ageRange lower="55" upper="100">0.18</ageRange>
        </ageRangeDistributions>
        <demographicDistributions>
            <gender MALE="0.49" FEMALE="0.51"/>
            <orientation MSW="0.925" MSM="0.053" MSMW="0.022"/>
            <raceAndEthnicityProfiles>
                <raceAndEthnicity type="WHITE:NON_HISPANIC">0.45</raceAndEthnicity>
                <!-- ... -->
            </raceAndEthnicityProfiles>
        </demographicDistributions>
    </entityDistributions>
</initialState>
```

**What it controls**: the initial population's size, age distribution, gender split, sexual-orientation split, and race × ethnicity composition.

**Inner mechanism**: At `Population::GenerateInitialEntities` time, the model draws `<size>` entities. For each entity, it samples:

1. Age bucket from `<ageRangeDistributions>` (proportions must sum to 1.0)
2. Gender from `<gender>` attributes
3. Sexual orientation (males only — females are always heterosexual)
4. Race × ethnicity from the listed `<raceAndEthnicity>` proportions

If `useCounts="true"`, the proportions become exact integer counts instead. The `<size>` then becomes the sum of counts across age buckets rather than a parameter.

**Practical notes**:
- All distributions in this block must sum to 1.0 (or to the explicit `<size>` if using counts)
- The age buckets here are independent of the age buckets in `<traceFiles>` outputs — they're for initial population only
- `MSW` = men who have sex with women (heterosexual), `MSM` = men who have sex with men (homosexual), `MSMW` = bisexual

### `<births>` — newborns entering each month

```xml
<births useBirthRate="false">
    <birthRate>0</birthRate>
    <fertilityRate>
        <rateForAgeRange lower="15" upper="44">0.0034</rateForAgeRange>
    </fertilityRate>
    <demographicDistributions>
        <gender MALE="0.49" FEMALE="0.51"/>
        <orientation MSW="0.925" MSM="0.053" MSMW="0.022"/>
        <raceAndEthnicityProfiles>
            <raceAndEthnicity type="WHITE:NON_HISPANIC">0.45</raceAndEthnicity>
        </raceAndEthnicityProfiles>
    </demographicDistributions>
</births>
```

**What it controls**: how many new entities are added each month, and their demographic mix.

**Inner mechanism**: Each month, the model decides how many newborns to add based on either:

- `useBirthRate="true"` + `<birthRate>` — a global per-person-per-month birth rate. Number of newborns = `birthRate × current_pop_size`.
- `useBirthRate="false"` + `<fertilityRate>` — age-stratified rates applied only to females in each age range. Number of newborns = `Σ (rate_for_age × num_females_in_age_range)`.

Each newborn samples age (always 0), gender, orientation, and race/ethnicity from `<demographicDistributions>` (same structure as in `<initialState>`). They join the population at month 0 of age and become sexually active at `<ageOfMajority>`.

**Practical note**: fertility-based birth tracks the female population more realistically (declining births if the population ages or shrinks), but requires up-to-date age-specific rates. Birth-rate-based is simpler for testing.

### `<initialInfections>` — seeding HIV-positive entities

```xml
<initialInfections>
    <delay>600</delay>
    <chanceSeedChronicInfection>0.95</chanceSeedChronicInfection>
    <useCoefficients>true</useCoefficients>
    <seedPrevalence>0.001</seedPrevalence>
    <profile bucket="SA:FEMALE:MSW:*:NON_CSW:WHITE:NON_HISPANIC" risk="low" age-range="17-49">
        ...
    </profile>
</initialInfections>
```

**What it controls**: when HIV is introduced into the population, and the demographic profile of the seed cases.

**Inner mechanism**:
- `<delay>` — burn-in period in months. Before this month, no entities are infected (population just ages).
- `<chanceSeedChronicInfection>` — for each seed case, probability they enter the simulation already in chronic infection (vs. acute primary).
- `<useCoefficients>` / `<seedPrevalence>` — if true, seed proportional to current population size at `<delay>` month. If false, seed exact counts per `<profile>`.
- `<profile>` — each one defines a demographic bucket (sexual-activity-status:gender:orientation:relationship-status:employment:race:ethnicity) with optional `risk` and `age-range`. The text content (or numeric attribute) is the count or proportion to seed in that profile.

`applyPrevalentInfection` runs at month `<delay>` and randomly selects entities matching each `<profile>` to seed.

**Practical notes**:
- The `*` wildcards in `bucket=` mean "any value for this dimension"
- Use multiple `<profile>` entries to seed across multiple demographic groups in one shot
- A bigger initial seed means faster epidemic ramp-up but is less realistic for the early years

### `<transmissionCoefficients>` — the per-act probabilities

```xml
<transmissionCoefficients>
    <maleToFemale>
        <hvl0>0.00005</hvl0>
        <hvl1>0.0001</hvl1>
        <hvl2>0.0005</hvl2>
        <hvl3>0.001</hvl3>
        <hvl4>0.002</hvl4>
        <hvl5>0.003</hvl5>
        <hvl6>0.005</hvl6>
        <primary>0.01</primary>
        <lateStage>0.008</lateStage>
    </maleToFemale>
    <femaleToMale> ... </femaleToMale>
    <maleToMale> ... </maleToMale>
</transmissionCoefficients>
```

**What it controls**: the **base per-coital-act transmission probability**, indexed by direction and the **infector's** viral load stratum.

**Inner mechanism**: These values feed directly into the FOI calculation in `Male::getFOI` and `Female::getFOI`. The lookup is:

```
baseFoi = transmission_coefficients[direction][infector_HVL]
```

Then condom, circumcision, microbicide, and PrEP multiplicative reducers are stacked on top. See [Transmission and Force of Infection](Mechanism-Transmission-and-Force-of-Infection.md).

**HVL strata** (CEPAC's classification):

| Tag | HVL range (copies/mL) | Stage |
|---|---|---|
| `hvl0` | 0–20 | Very low |
| `hvl1` | 21–500 | Low |
| `hvl2` | 501–3,000 | Mid-low |
| `hvl3` | 3,001–10,000 | Mid |
| `hvl4` | 10,001–30,000 | Mid-high |
| `hvl5` | 30,001–100,000 | High |
| `hvl6` | 100,000+ | Very high |
| `primary` | n/a | Acute infection (first ~3 months) |
| `lateStage` | n/a | AIDS / late-stage disease |

**Practical notes**:
- The infector's HVL is read from CEPAC each month. As an infected entity progresses from acute to chronic to late-stage, the per-act probability changes accordingly.
- These are the most-tuned parameters in calibration. If your incidence curve is wrong, this is the first place to look.
- The three direction-pairs are independent — you can have asymmetric M→F vs F→M probabilities (which biologically you should).

### `<preExposureProphylaxis>` — PrEP module

```xml
<preExposureProphylaxis>
    <prepEfficacy>0.96</prepEfficacy>
    <prepDefaults>
        <prepEligibility>
            <partnerCount>2</partnerCount>
            <partnerStatus>POSITIVE</partnerStatus>
            <partnerRiskLevel>HIGH</partnerRiskLevel>
            <monthsSinceUnprotectedAct>6</monthsSinceUnprotectedAct>
        </prepEligibility>
        <prepAccess>0.5</prepAccess>
        <prepAdherence> ... </prepAdherence>
        <prepRetention> ... </prepRetention>
        <prepReturnToCare> ... </prepReturnToCare>
    </prepDefaults>
    <prepProfiles>
        <demographic bucket="SA:MALE:MSW:*:*:WHITE:NON_HISPANIC">
            <prepAccess>0.6</prepAccess>
            <!-- overrides defaults for this demographic -->
        </demographic>
    </prepProfiles>
</preExposureProphylaxis>
```

**What it controls**: PrEP eligibility criteria, access rates, adherence levels, and demographic-specific overrides.

**Inner mechanism**:
- `<prepEfficacy>` — the per-act protection level when adherent. Applied as `(1 - prepEfficacy × adherence_multiplier)` in the FOI.
- `<prepDefaults>` apply to all eligible entities unless overridden.
- `<prepEligibility>` — criteria an entity must meet to start PrEP (number of partners, partner HIV status, partner risk level, recent unprotected sex).
- `<prepAccess>` — probability of accessing PrEP when eligible.
- `<prepAdherence>` — probability distribution over the four adherence levels (PREP_ADHERENT, PREP_SUBSTANTIALLY, PREP_PARTIALLY, PREP_INADHERENT, PREP_OFF_PREP).
- `<prepRetention>` and `<prepReturnToCare>` — monthly probabilities of staying on / returning to PrEP.
- `<prepProfiles>` — demographic-specific overrides. Each `<demographic bucket=...>` defines a profile; matching entities use these values instead of the defaults.

PrEP adherence is modeled with multipliers on the efficacy. See [Transmission and Force of Infection → PrEP](Mechanism-Transmission-and-Force-of-Infection.md#male-to-female-and-male-to-male).

### `<entities>` — per-entity-type configuration

```xml
<entities>
    <entity type="male">
        <allowedRaceAndEthnicities> ... </allowedRaceAndEthnicities>
        <behavior> ... </behavior>
        <health> ... </health>
    </entity>
    <entity type="female"> ... </entity>
    <entity type="msm"> ... </entity>
    <entity type="msmw"> ... </entity>
</entities>
```

Each `<entity>` block carries the parameters that apply to **that orientation type**. Male and female differ substantially (males are partnership initiators); MSM/MSMW reuse most of the male behavior structure but with different partnership types.

#### `<entity type="male"> <behavior>` — partnership behavior

```xml
<behavior>
    <cswEndAge>50</cswEndAge>
    <chanceBecomeSexWorker>0.01</chanceBecomeSexWorker>
    <partnerAcqMultWithSteadyHighRisk>0.5</partnerAcqMultWithSteadyHighRisk>
    <partnerAcqMultWithSteadyLowRisk>0.3</partnerAcqMultWithSteadyLowRisk>
    <highRiskAcqRateMultiplier enabled="true">2.0</highRiskAcqRateMultiplier>
    <proportionHighRiskCsw>0.95</proportionHighRiskCsw>
    <proportionHighRiskNonCsw>0.15</proportionHighRiskNonCsw>
    <chanceMsmwChooseMale>0.5</chanceMsmwChooseMale>
    <chanceMsmChooseMsmw>0.3</chanceMsmChooseMsmw>
    <ageDiscounting>
        <startAgeYrs>40</startAgeYrs>
        <acquisitionDiscByYr>0.95</acquisitionDiscByYr>
        <coitalActsDiscByYr>0.97</coitalActsDiscByYr>
    </ageDiscounting>
    <assortivity>
        <riskAssortivity>0.85</riskAssortivity>
        <raceEthnicAssortivity baseline="1.0" useCoefficients="false">0.95</raceEthnicAssortivity>
    </assortivity>
    <partnershipTypes>
        <partnership type="steady"> ... </partnership>
        <partnership type="regular"> ... </partnership>
        <partnership type="casual"> ... </partnership>
        <partnership type="csw"> ... </partnership>
    </partnershipTypes>
    <maxPartnershipRejections>5</maxPartnershipRejections>
</behavior>
```

**What it controls**: every aspect of how males form, conduct, and end partnerships.

**Inner mechanism**: These parameters seed the population-level distributions from which each individual male draws his personal behavior at birth.

| Sub-block | Used in |
|---|---|
| `<cswEndAge>` | Forces CSWs to retire from sex work at this age |
| `<chanceBecomeSexWorker>` | Probability of joining the CSW pool at sexual debut |
| `<partnerAcqMultWithSteady*>` | Damping factor on non-steady partner acquisition when the male already has a steady partner — see `rollForNumPartners` in [Partnership Formation](Mechanism-Partnership-Formation.md) |
| `<proportionHighRiskCsw>` / `<proportionHighRiskNonCsw>` | Fraction of each subgroup assigned to high-risk behavior at birth |
| `<chanceMsmwChooseMale>` / `<chanceMsmChooseMsmw>` | For bisexual / MSM males, probability that any given partnership goes to a male partner |
| `<ageDiscounting>` | Older males acquire fewer partners and have fewer coital acts; `<startAgeYrs>` is the threshold, the multipliers are applied per year past it |
| `<assortivity>` | Probability of choosing a partner of the same risk level / same race-ethnicity |

The big nested block is `<partnershipTypes>`. Each `<partnership type="...">` configures one of: `steady`, `regular`, `casual`, `csw` (for heterosexual males) or `steadyMSM`, `regularMSM`, `casualMSM`, `cswMSM` (for MSM/MSMW). Each `partnership` has:

```xml
<partnership type="steady">
    <BreakupRateForMSW>0.01</BreakupRateForMSW>
    <BreakupRateForMSM>0.02</BreakupRateForMSM>
    <selectionCriteria>
        <chanceChooseWithSteady>0.6</chanceChooseWithSteady>
        <averageYearsYounger>
            <distribution type="normal">
                <mean>3.0</mean>
                <stdDev>2.0</stdDev>
            </distribution>
        </averageYearsYounger>
    </selectionCriteria>
    <lowRisk>
        <acquisitionRate>
            <distribution type="normal"><mean>0.05</mean><stdDev>0.02</stdDev></distribution>
        </acquisitionRate>
        <coitalEventsPerMonth>
            <distribution type="poisson"><mean>5</mean></distribution>
        </coitalEventsPerMonth>
        <chanceCondomUsePerEvent>
            <distribution type="normal"><mean>0.4</mean><stdDev>0.15</stdDev></distribution>
        </chanceCondomUsePerEvent>
        <partnershipDurationMth>
            <distribution type="shifted-log-normal">
                <mean>36</mean><stdDev>24</stdDev><shift>1</shift>
            </distribution>
        </partnershipDurationMth>
    </lowRisk>
    <highRisk> ... </highRisk>
</partnership>
```

**What this controls**: every behavioral parameter that distinguishes one partnership type from another, separated by risk level.

**Inner mechanism**: At birth, each male draws his personal value of each parameter from these population distributions. Once drawn, they're fixed for life (unless an intervention overrides them).

- `<BreakupRateForMSW>` / `<BreakupRateForMSM>` — monthly probability the partnership dissolves. `0.01` ≈ 1% chance per month, so the partnership has a half-life of ~70 months.
- `<chanceChooseWithSteady>` — for non-CSW non-steady partnerships, probability the chosen partner is themselves in a steady relationship
- `<averageYearsYounger>` — age preference (positive = prefer younger partners)
- `<acquisitionRate>` — Poisson rate per month for forming new partnerships of this type
- `<coitalEventsPerMonth>` — Poisson rate of coital events within an existing partnership
- `<chanceCondomUsePerEvent>` — Beta distribution mean & stddev (converted from normal) for the per-act condom-use probability
- `<partnershipDurationMth>` — shifted log-normal for partnership duration (in months). The `shift` adds a minimum duration.

**Distribution types** (parsed in `simulationparametersxml.cpp`):

| `type=` | Parameters | Used for |
|---|---|---|
| `normal` | `mean`, `stdDev` | Most behavior params, age preference |
| `poisson` | `mean` | Count distributions (acts, partners) |
| `log-normal` | `mean`, `stdDev` | Acquisition rates (always positive, right-skewed) |
| `shifted-log-normal` | `mean`, `stdDev`, `shift` | Partnership durations (minimum duration via shift) |
| `beta` | `alpha`, `beta` | Condom-use chance (bounded [0,1]) |

Normal distributions targeting a [0,1] parameter (like condom use) are internally converted to Beta via `BetaDist::FromNormal` (utility/randomnumbergenerator.hpp).

#### `<entity type="male"> <health>`

```xml
<health>
    <proportionMaleCircumcised>0.3</proportionMaleCircumcised>
    <circumcisionProtectEfficacy>0.6</circumcisionProtectEfficacy>
    <condomProtectEfficacy>0.85</condomProtectEfficacy>
    <preExposureProphylaxisEfficacy>0.96</preExposureProphylaxisEfficacy>
</health>
```

**What it controls**: efficacy of the per-act protection mechanisms.

**Inner mechanism**:
- `<proportionMaleCircumcised>` — at birth, each male is circumcised with this probability. The `circumcise` intervention can change this dynamically.
- `<circumcisionProtectEfficacy>` — multiplicative reduction in transmission per act when the susceptible male is circumcised (F→M direction only).
- `<condomProtectEfficacy>` — multiplicative reduction when a condom is used. Same for both directions.
- `<preExposureProphylaxisEfficacy>` — full-adherence PrEP efficacy (later scaled by adherence multipliers).

See [Transmission and Force of Infection](Mechanism-Transmission-and-Force-of-Infection.md) for the full FOI formula.

#### `<entity type="female">`

Females have a much simpler `<behavior>` block — no partnership-type config, since they don't initiate partnerships. Only:

```xml
<entity type="female">
    <behavior>
        <cswEndAge>50</cswEndAge>
        <chanceBecomeSexWorker>0.02</chanceBecomeSexWorker>
        <proportionHighRiskCsw>0.95</proportionHighRiskCsw>
        <proportionHighRiskNonCsw>0.15</proportionHighRiskNonCsw>
    </behavior>
    <health>
        <preExposureProphylaxisEfficacy>0.85</preExposureProphylaxisEfficacy>
        <vaginalMicrobicideEfficacy>0.55</vaginalMicrobicideEfficacy>
    </health>
</entity>
```

Female partnership parameters (coital frequency, condom use, etc.) are read **from the male partner** — see the asymmetry in the [Transmission mechanism page](Mechanism-Transmission-and-Force-of-Infection.md#female-to-male).

#### `<entity type="msm">` and `<entity type="msmw">`

Same structure as the male `<behavior>` block but with `<partnership type="steadyMSM">`, `<partnership type="regularMSM">`, etc. — the four MSM-specific partnership types. MSMW entities have both heterosexual AND MSM partnership types (all 8); MSM-only entities have only the 4 MSM types.

---

## `<interventions>` — what changes mid-simulation

```xml
<interventions>
    <cepacIntervention enabled="true"> ... </cepacIntervention>
    <artRolloutIntervention enabled="true"> ... </artRolloutIntervention>
    <populationInterventions> ... </populationInterventions>
    <groups> ... </groups>
</interventions>
```

### `<cepacIntervention>` — disease-progression engine

```xml
<cepacIntervention enabled="true">
    <cepacTreatmentFiles>
        <treatmentFile>
            <time>0</time>
            <fileName>07212021B_NoART.in</fileName>
            <fileNumber>0</fileNumber>
        </treatmentFile>
    </cepacTreatmentFiles>
</cepacIntervention>
```

**What it controls**: which CEPAC `.in` file(s) drive disease progression for HIV-positive entities.

**Inner mechanism**: Each `<treatmentFile>` is loaded into a CEPAC `SimContext` at startup and stored in `EventParams::cepacSimContexts`. Each `Entity` receives a CEPAC `Patient` that points to the appropriate context. When `<time>` advances past a switch point, entities optionally migrate to a new context.

For the full coupling story, see [CEPAC Integration](Mechanism-CEPAC-Integration.md).

**Practical notes**:
- The `<fileName>` is a path relative to the directory passed as `--cepac` on the command line.
- `<fileNumber>` must be unique among all CEPAC files in the XML.
- If `artRolloutIntervention` is enabled, that block's `<rolloutTreatmentFiles>` is used instead of `cepacTreatmentFiles`.

### `<artRolloutIntervention>` — ART access and coverage

```xml
<artRolloutIntervention enabled="true">
    <dynamicTreatmentScaling enabled="true">
        <feedbackPeriod>-1</feedbackPeriod>
    </dynamicTreatmentScaling>
    <rolloutTreatmentFiles>
        <rolloutFile>
            <time>0</time>
            <fileName>07212021B_NoART.in</fileName>
            <fileNumber>0</fileNumber>
            <popToApply>0</popToApply>
        </rolloutFile>
        <!-- more files for treated populations -->
    </rolloutTreatmentFiles>
    <rolloutEligibility enabled="false">
        <criteria name="OIHist"> ... </criteria>
    </rolloutEligibility>
    <targetRolloutProportions>
        <target year="1989" value="0.0">...</target>
        <target year="2001" value="0.01">
            <demographicDistributions>
                <gender MALE="0.486" FEMALE="0.514"/>
                <orientation MSW="0.92" MSM="0.07" MSMW="0.01"/>
                <raceAndEthnicityProfiles>
                    <raceAndEthnicity type="WHITE:NON_HISPANIC">0.13</raceAndEthnicity>
                </raceAndEthnicityProfiles>
            </demographicDistributions>
        </target>
    </targetRolloutProportions>
</artRolloutIntervention>
```

**What it controls**: the full ART rollout cascade — who gets eligible, who accesses, how the target coverage scales over time.

**Inner mechanism**:

- `<dynamicTreatmentScaling>` with `<feedbackPeriod>` — every N months, if actual coverage is below the target, the model increases the "eligible for access" pool. `-1` disables this feedback.
- `<rolloutTreatmentFiles>` — each `<rolloutFile>` is a CEPAC `.in` file with a `<time>` when it activates and a `<popToApply>` (0 = untreated, 1 = treated, 2 = newly treated, -1 = none). When the time arrives, matching entities switch to this `SimContext`.
- `<rolloutEligibility>` — criteria for ART eligibility (CD4, OI history, HVL). Configured with `<criteria>` blocks each holding multiple ranked criteria (see the `KnownIntervention::rolloutEligibility` section below). Can also be changed mid-simulation via a population intervention.
- `<targetRolloutProportions>` — for each calendar year, the target proportion of eligible HIV+ entities on treatment. The `<demographicDistributions>` block stratifies the target across gender, orientation, race/ethnicity (see [Tutorial](https://github.com/hsphcdm/transm/blob/develop/doc/Tutorial.md)).

For ART rollout details, see [CEPAC Integration → ART rollout](Mechanism-CEPAC-Integration.md#art-rollout-switching-simcontexts).

### `<populationInterventions>` — scheduled population-wide changes

```xml
<populationInterventions>
    <chanceCondomUse risk="low" type="steady" time="700" duration="120">
        <distribution type="normal"><mean>0.7</mean><stdDev>0.1</stdDev></distribution>
        <transform>true</transform>
    </chanceCondomUse>
    <proportionCircumcised time="700">
        <proportion>0.6</proportion>
        <transform>true</transform>
    </proportionCircumcised>
</populationInterventions>
```

**What it controls**: parameter changes that apply to the whole population at a specific simulation month.

**Inner mechanism**: Each child element is a "known intervention" — a tag from a fixed list, each of which dispatches to a specific callback in the parser. The callback rewrites the relevant parameter in `PopulationParameters` and (optionally) on every existing entity.

Most interventions take:
- `time` — the simulation month when the intervention fires
- `duration` (optional) — over this many months, smoothly transition to the new value (linear interpolation)
- `<transform>true</transform>` — explicitly enable the gradual transformation

**Catalog of population intervention tags**:

| Tag | Effect | Parameters |
|---|---|---|
| `birthRate` | Replaces global birth rate | Text: rate value |
| `fertilityRate` | Replaces age-stratified fertility rates | `<rateForAgeRange>` children |
| `proportionMale` | Proportion of newborns who are male | Text: 0–1 |
| `proportionCircumcised` | Baseline circumcision rate | `<proportion>` + optional `<transform>` |
| `chanceBecomeSexWorker` | Probability of becoming CSW (per gender) | `gender=` attribute |
| `delaySexualActivity` | Delay sexual debut by N months | Text: months |
| `proportionHighRisk` | Proportion in high-risk behavior | `gender`, `employment` attributes |
| `averageYearsYounger` | Age-gap preference for males | `type=` partnership type + `<distribution>` |
| `partnerAcquisitionRate` | Partner acquisition rate | `risk`, `type` + `<distribution>` |
| `coitalEventsPerMonth` | Coital frequency | `risk`, `type` + `<distribution>` |
| `chanceCondomUse` | Condom-use chance per act | `risk`, `type` + `<distribution>` |
| `partnershipDuration` | Partnership length | `risk`, `type` + `<distribution>` |
| `rolloutEligibility` | ART eligibility criteria | Same nested structure as in `<artRolloutIntervention>` |

### Individual-level interventions (within groups / target groups)

```xml
<groups>
    <group label="treatment_arm_2025">
        <enrollment-period>900-1000</enrollment-period>
        <permanent-effect>true</permanent-effect>
        <open-enrollment>false</open-enrollment>
        <eligibility-criteria>
            <gender>male</gender>
            <hiv-status>negative</hiv-status>
            <age>17-19</age>
        </eligibility-criteria>
        <partitions>
            <partition>
                <proportion>0.5</proportion>
                <trace>true</trace>
                <interventions>
                    <circumcise time="900"/>
                </interventions>
            </partition>
            <partition>
                <proportion>0.5</proportion>
                <trace>true</trace>
                <interventions/>
            </partition>
        </partitions>
    </group>
</groups>
```

**What it controls**: experimental-style cohorts — define an enrollment window and eligibility criteria, split the eligible pool into partitions, apply different interventions per partition. Models randomized trials / cohort studies.

**Inner mechanism**: Entities matching `<eligibility-criteria>` during `<enrollment-period>` are randomly assigned to a partition based on `<proportion>` weights. Each partition's `<interventions>` are applied to its members. With `<permanent-effect>true</permanent-effect>`, effects persist after the enrollment window closes.

**Catalog of individual intervention tags** (use inside `<partition><interventions>`):

| Tag | Effect |
|---|---|
| `circumcise` | Circumcise this male |
| `chanceBecomeSexWorker` | Set/override CSW probability |
| `delaySexualActivity` | Postpone sexual debut |
| `averageYearsYounger` | Override age preference |
| `partnerAcquisitionRate` | Override acquisition rate |
| `coitalEventsPerMonth` | Override coital frequency |
| `chanceCondomUse` | Override condom-use chance |
| `partnershipDuration` | Override partnership length |
| `partnershipRejectionChance` | Probability of rejecting partnership offer |
| `overrideChanceCondomUse` | Permanent fixed condom-use override |
| `cepacContext` | Switch this entity to a different CEPAC `SimContext` |
| `vaginalMicrobicideAdherence` | Set female microbicide adherence |
| `preExposureProphylaxisAdherence` | Set PrEP adherence (HIV-negative only) |

---

## FOCUS screening (v4.8)

Not configured in the XML — toggled on the command line:

```bash
./bin/transm-v4.8.0 --cepac INFILES --focus focus_test.xml
```

When `--focus` is enabled, the simulation runs the FOCUS module each year: race × sex-stratified focused screening of undiagnosed and lost-to-follow-up HIV+ populations. Probabilities and per-group target counts are read from internal calibration tables — see the FOCUS source code in `population.cpp` for the current parameters.

The module covers **2017 through 2035** (simulation months 720–947 under the module's internal calendar, which treats month 600 as January 2007). 2017–2024 are observed program data; 2025–2035 are projections. Make sure `<duration>` is at least 948, or the later projection years will never run.

The source table's race “Other” has no slot in the twelve-cohort schema and is **folded into Hispanic**, so the Hispanic targets are source Hispanic + Other. The source “Other” female counts are zero in every year, so only the male Hispanic cohorts are affected. The source “Other Gender” category is not modeled at all — `DemographicProfile::Gender` is `{Male, Female}`.

Future versions may expose these through the XML.

### Seeing FOCUS in the network export

FOCUS state is written into the partnership network GraphML as node attributes
(`focus_selected`, `focus_status`, `focus_group_str`, …) and one edge attribute
(`focus_edge`), for inspection in Gephi. See
[Outputs-Reference](Outputs-Reference.md#partnershipnetwork_monthgraphml) for the
full attribute list.

The dumps are XML-driven but the FOCUS window is fixed in code, so the two have
to be lined up by hand:

```xml
<partnerNetwork enabled="true" tossIfCalibFail="false">
  <time>720</time>   <!-- Jan 2017, the month FOCUS starts -->
  <time>840</time>
  <time>947</time>   <!-- Dec 2035, the last month FOCUS runs -->
</partnerNetwork>
```

A `<time>` outside `[720, 948)` produces a snapshot whose FOCUS columns are
uniformly empty, and any `<time>` beyond `<duration>` never fires at all.

---

## Conventions and tips

### Template parameters: `@variable_name`

Placeholders like `@condom_use_mult_B` in the XML are **not parsed by transm itself**. An external pre-processing script substitutes them with concrete values before the XML is fed to transm. This is how a single template generates hundreds of calibration runs.

### `<parameter key="$key_name">value</parameter>` — for general interventions

```xml
<chanceCondomUsePerEventHighRisk>
    <distribution type="normal">
        <mean><parameter key="$0">0.5</parameter></mean>
        <stdDev><parameter key="$1">0.25</parameter></stdDev>
    </distribution>
</chanceCondomUsePerEventHighRisk>
```

The `<parameter key="$0">` element makes the value addressable from `<populationInterventions>` so an intervention can change it mid-simulation:

```xml
<populationInterventions>
    <intervention time="834" key="$0" value="0.75"/>
    <intervention time="834" key="$1" value="0.25"/>
</populationInterventions>
```

Both parameters of a distribution must be updated together — the simulation will error out if you only update one.

### Time units

| Field | Unit |
|---|---|
| `<duration>`, `<delay>`, `time=` attributes | **months** (simulation time) |
| `year=` attribute on `<target>` | **calendar year** (anchored by `<monthOf1990>`) |
| `<ageOfMajority>`, `<cswEndAge>`, `<startAgeYrs>` | **years** |
| `<partnershipDurationMth>` | **months** (explicit) |
| `<acquisitionRate>`, `<coitalEventsPerMonth>` | **per month** |

### Demographic bucket strings

The `bucket=` attribute in `<profile>`, `<demographic>`, and `<eligibility-criteria>` uses a colon-delimited 7-field format:

```
SexualActivityStatus : Gender : SexualOrientation : RelationshipStatus : Employment : Race : Ethnicity
```

Use `*` as a wildcard for any field. Valid values:

- `SexualActivityStatus`: `SA`, `NA`
- `Gender`: `MALE`, `FEMALE`
- `SexualOrientation`: `MSW`, `MSM`, `MSMW`
- `RelationshipStatus`: `SINGLE`, `NON_SINGLE`
- `Employment`: `CSW`, `NON_CSW`
- `Race`: `WHITE`, `BLACK`, `OTHER`
- `Ethnicity`: `HISPANIC`, `NON_HISPANIC`

Example: `SA:MALE:MSW:*:NON_CSW:WHITE:NON_HISPANIC` matches sexually-active, heterosexual, non-CSW, white-non-Hispanic males regardless of relationship status.

### Race-specific multipliers (v4.8 status)

Two new XML parameters in v4.8:

```xml
<populationParameters>
    <multiplierCondomUseBlacks>@condom_use_mult_B</multiplierCondomUseBlacks>
    <multiplierCondomUseWhites>@condom_use_mult_W</multiplierCondomUseWhites>
</populationParameters>
```

Missing nodes default to 1.0 (no multiplier). Used by `Male::updateBetaDistForRace` to rescale a male's per-event condom-use Beta distribution by race.

Note: the race-specific **coital event** multipliers (5× black, 0.33× white, 1.33× hispanic on Steady/Regular) are still **hardcoded** in `male.cpp:524-541` as of v4.8 — Issue #91 tracks the parameterization.

### Common gotchas

- **Numbers vs proportions**: `useCounts` toggles between absolute counts and proportions. Mismatched and you'll get nonsensical population sizes.
- **`enabled="false"` doesn't always mean ignored**: some blocks still get parsed for defaults. Read the parser code if a "disabled" intervention seems to be affecting your run.
- **Time units in `<targetRolloutProportions>`**: the `year=` attribute is a calendar year, not a simulation month. The simulation converts via `<monthOf1990>`.
- **Adding a new parameter? Three places**: declare it in `PopulationParameters`, parse it in `simulationparametersxml.cpp`, use it where needed. See [Developer's Guide → adding a parameter](Developers-Guide.md#worked-example-adding-a-new-parameter).

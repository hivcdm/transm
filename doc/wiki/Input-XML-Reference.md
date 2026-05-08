# Input XML Reference

> **Status: in progress.** This page lists the top-level XML structure as of v4.8. The deep parameter-by-parameter explanation is being filled in incrementally; check the relevant section in [`simulationparametersxml.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp) for ground truth.

The reference scenario for v4.8 is `MIAMI-prime/focus_test.xml`. Use it as a template; copy and modify rather than starting from scratch.

## Top-level structure

```xml
<simulation version="4.5">
    <debugLevel>0</debugLevel>
    <duration>1000</duration>            <!-- months -->
    <fixedSeed>0</fixedSeed>              <!-- 0=default, -1=time, >0=specific -->
    <monthOf1990>600</monthOf1990>

    <traceFiles> ... </traceFiles>
    <concurrencyDefinition> ... </concurrencyDefinition>
    <population> ... </population>
    <interventions> ... </interventions>
    <calibration enabled="false"> ... </calibration>
</simulation>
```

The `version=` attribute on `<simulation>` is informational; it doesn't gate parser behavior.

## `<traceFiles>`

Enables/disables every per-month trace file the simulation can produce. Each child element corresponds to a `TraceFile::Type` value in [`eventparams.hpp`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/eventparams.hpp).

```xml
<traceFiles>
    <population        enabled="true"  tossIfCalibFail="false"><extension>Population.xls</extension></population>
    <infection         enabled="true"  tossIfCalibFail="false"><extension>Infections.xls</extension></infection>
    <partnership       enabled="true"  tossIfCalibFail="false"><extension>Partnership.xls</extension></partnership>
    <survival          enabled="false" tossIfCalibFail="true"><extension>Survival.xls</extension></survival>
    <costEffectiveness enabled="false" tossIfCalibFail="true">
        <extension>CE.xls</extension>
        <condomCost>0.5</condomCost>
        <circumcisionCost>500</circumcisionCost>
        <prEPCost>1</prEPCost>
        <vaginalMicrobicideCost>1</vaginalMicrobicideCost>
    </costEffectiveness>
    <clinical          enabled="true"  tossIfCalibFail="true" ><extension>Clinical.xls</extension></clinical>
    <events            enabled="false" tossIfCalibFail="true"><extension>Events.xls</extension></events>
    <singlePerson      enabled="false" tossIfCalibFail="true">
        <extension>SinglePerson.xls</extension>
        <numberToTracePerAgeRange>15</numberToTracePerAgeRange>
        <numberNewbornsToTrace>500</numberNewbornsToTrace>
        <monthTraceNewborns>600</monthTraceNewborns>
        <tracePrevalentCases>1</tracePrevalentCases>
    </singlePerson>
    <prepOutcomes      enabled="true"  tossIfCalibFail="false"><extension>PrepOutcomes.xls</extension></prepOutcomes>
    <calibrationStatistics enabled="true" tossIfCalibFail="false"><extension>CalibStats.xls</extension></calibrationStatistics>
    <artRollout        enabled="true"  tossIfCalibFail="false"><extension>ARTRollout.xls</extension></artRollout>
    <!-- ... -->
</traceFiles>
```

`tossIfCalibFail="true"` means the trace file is deleted at end-of-run if the population didn't pass partnership-formation calibration.

## `<population>`

Sets initial population size, age structure, and entity-type proportions.

Key sub-elements (incomplete):

- `<populationSize>` — total entities at simulation start
- `<initialAgeBuckets>` — age-bracket proportions
- `<entityTypes>` — proportions of male / female / msm / msmw at birth and in the initial population
- `<allowedRaceAndEthnicities>` — which (race, ethnicity) combinations exist
- `<assortivity>` — by-race, by-ethnicity assortativity probabilities
- `<chanceSeedChronicInfection>` — fraction of initial-prevalent cases seeded as chronic vs primary

The full set of sub-elements is most easily understood by reading `GetPopulationParameters` in [`simulationparametersxml.cpp`](https://github.com/hsphcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp).

## `<interventions>`

Each child is an intervention type — most are toggled with `enabled="true|false"`.

```xml
<interventions>
    <cepacIntervention enabled="true">
        <cepacTreatmentFiles>
            <treatmentFile>
                <time>0</time>
                <fileName>07212021B_NoART.in</fileName>
                <fileNumber>0</fileNumber>
            </treatmentFile>
        </cepacTreatmentFiles>
    </cepacIntervention>

    <artRolloutIntervention enabled="true">
        <dynamicTreatmentScaling enabled="true">
            <feedbackPeriod>-1</feedbackPeriod>
        </dynamicTreatmentScaling>
        <rolloutTreatmentFiles>
            <rolloutFile>
                <time>0</time>
                <fileName>07212021B_NoART.in</fileName>
                <!-- ... -->
            </rolloutFile>
        </rolloutTreatmentFiles>
        <targetRolloutProportions>
            <target year="2001" value="0.01">
                <demographicDistributions>
                    <gender MALE="0.486" FEMALE="0.514"/>
                    <orientation MSW="0.92" MSM="0.07" MSMW="0.01"/>
                </demographicDistributions>
            </target>
            <!-- ... -->
        </targetRolloutProportions>
    </artRolloutIntervention>

    <generalInterventions>
        <intervention key="$proportion_high_risk" time="700" value="0.2" target-gender="male"/>
        <!-- ... -->
    </generalInterventions>
</interventions>
```

For all the intervention types, see [`KnownIntervention` enum](https://github.com/hsphcdm/transm/blob/develop/src/parameters/simulationparametersxml.cpp#L1410) and the giant switch around line 1592.

## `<calibration>`

Calibration mode — when `enabled="true"`, runs are filtered by their final partnership-formation statistics matching specified ranges.

```xml
<calibration enabled="true">
    <numActs>
        <popOfInterest>2</popOfInterest>      <!-- partnership-type index -->
        <lwrBound>1.5</lwrBound>
        <uprBound>3.0</uprBound>
    </numActs>
    <!-- ... -->
</calibration>
```

If a run doesn't pass, trace files marked `tossIfCalibFail="true"` are deleted.

## v4.8 race-specific knobs

Two new XML parameters land in v4.8 (from Issue #89):

```xml
<populationParameters>
    <multiplierCondomUseBlacks>@condom_use_mult_B</multiplierCondomUseBlacks>
    <multiplierCondomUseWhites>@condom_use_mult_W</multiplierCondomUseWhites>
    <!-- ... -->
</populationParameters>
```

These rescale the male's per-event condom-use Beta distribution by race. Missing nodes default to 1.0 (no change). See [Modeler's Guide → race-specific multipliers](Modelers-Guide.md#race-specific-multipliers-v48-status).

The race-specific *coital event* multipliers (5× for Black, 0.33× for White, 1.33× for Hispanic, on Steady/Regular partnerships) are **not yet XML-parameterized** in v4.8 — they're hardcoded in [`male.cpp:524-541`](https://github.com/hsphcdm/transm/blob/develop/src/entities/male.cpp#L524). Issue #91 tracks the parameterization.

## Templating with `@variable_name`

XML files are pre-processed by an external script (not part of transm) that substitutes `@variable_name` placeholders with concrete values. This is how a single XML template generates hundreds of calibration runs. Transm itself doesn't process `@` — values are substituted before transm sees the file.

## Editing tips

- Open `focus_test.xml` and `MIAMI_template.xml` side-by-side to see what's calibration-specific vs scenario-specific.
- Search for `parameter key="$"` in the XML — these are template parameters that interventions can target.
- If you're adding a new XML element, add a parser entry in `simulationparametersxml.cpp` AND mention it on this page.

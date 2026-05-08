# Outputs Reference

> **Status: in progress.** Lists every output file and the most-asked-about columns. Full column-by-column docs are filled in incrementally.

A typical run produces ~10 trace files plus several summary files in the same directory as the XML.

## Per-month trace files (one row per simulation month)

| File | Source | Toggle |
|---|---|---|
| `<sim>-Population.xls` | [`Population::PrintPopulation`](https://github.com/hsphcdm/transm/blob/develop/src/core/population.cpp) | `<traceFiles><population enabled="true">` |
| `<sim>-Infections.xls` | [`InfectionsTracker::printInfections`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/infectionstracker.cpp#L452) | `<infection enabled="true">` |
| `<sim>-Partnership.xls` | `Population::PrintPartnerships` | `<partnership enabled="true">` |
| `<sim>-Clinical.xls` | `Population::PrintClinical` | `<clinical enabled="true">` |
| `<sim>-CalibStats.xls` | calibration tracker | `<calibrationStatistics enabled="true">` |
| `<sim>-ARTRollout.xls` | [`ArtRolloutTracker`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/artrollouttracker.cpp) | `<artRollout enabled="true">` |
| `<sim>-PrepOutcomes.xls` | [`PrepTracker`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/preptracker.cpp) | `<prepOutcomes enabled="true">` |
| `<sim>-Survival.xls` | survival stats | `<survival enabled="true">` |
| `<sim>-CE.xls` | cost-effectiveness | `<costEffectiveness enabled="true">` |
| `<sim>-SinglePerson.xls` | single-person trace | `<singlePerson enabled="true">` |

## End-of-run summary files

| File | Source |
|---|---|
| `<sim>-summaryStats` | `printSummaryStats` |
| `<sim>-IndividualSummaries.json` | `Population::SaveIndividualSummaries` |
| `<sim>-InterventionOutcomes.xls` | `outputs_.intervention_outcomes.Write` |
| `cepacPopstats.out` | CEPAC's RunStats |
| `cepacRunStats` | CEPAC's RunStats |

## `Infections.xls` columns

The most-referenced trace file. Header rows (3 of them — group, sub-group, leaf-name). Body rows are one per month.

### Top-level groups (left to right)

1. **Epidemiology Outputs** — month, new infections, total, prevalence, etc.
2. **Prevalent Cases** — currently-alive HIV-positive counts by gender / orientation / age / risk / generation
3. **Exposures by HVL** — counts of coital events between discordant pairs
4. **Incident Cases** — new infections this month, by various stratifications
5. **Total Infected in History** — cumulative across the whole run
6. **Age at Infection** — mean and SD of age-at-infection by demographics
7. **Condom Use** — total exposures, condoms used, by partnership type

### Key columns to know

| Column | Source counter | Type | Note |
|---|---|---|---|
| `Month` | `time_.in_months()` | scalar | 0 at simulation start, "init" before |
| `New Infections` | `currTimeStepIncidentInfs[HVL]` summed | flow (this month) | Resets each month |
| `Total Infected in History (Prevalent Cases Excluded)` | `totalIncidentInfections[HVL]` summed | cumulative | Never resets |
| `Currently Infected` | `currPrevalentInfections[gen][profile]` summed | stock | Recomputed each month |
| `Pop Size` | `Population::GetSize()` | stock | |
| `Prevalence` | `currentlyInfected / popSize` | rate | |
| `Past-Year Pop Incidence` | rolling 12-month sum | rate | |
| `MALE` / `FEMALE` (under "Incident Cases / Entity Type") | `currTimeStepNumInfectedGender` | flow | **post-v4.8 fix** — sums to `New Infections` |
| `MALE` / `FEMALE` (under "Total Infected in History / Entity Type") | `totalIncidentInfsGender` | cumulative | Sums to `Total Infected in History` |
| `MALE (By Age)` / `FEMALE (By Age)` | `currTimeStepIncidentInfsGenderAge` | flow | Sum equals `New Infections` |

### Invariants you can check

For every row of `Infections.xls`:

- `Male (Incident Entity Type)` + `Female (Incident Entity Type)` **==** `New Infections`
- `Sum over age of Male (By Age)` + `Sum over age of Female (By Age)` **==** `New Infections`
- `Male (Cumulative Entity Type)` + `Female (Cumulative Entity Type)` **==** `Total Infected in History`

If these don't hold, there's a bug in the tracker — file an issue.

### Common pitfall: comparing flow vs stock

The `New Infections` column is **monthly flow**. The cumulative `Male` / `Female` columns are **lifetime accumulation**. They don't equal each other except in month 1. See the [v4.8 calibration thread for context](Modelers-Guide.md#common-mistakes).

## ART rollout file

`<sim>-ARTRollout.xls` has columns for HIV tests offered/accepted, eligible-for-access, accessing treatment, eligible-for-treatment (per CEPAC), and receiving treatment — broken down by demographic profile. Driven by [`ArtRolloutTracker`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/artrollouttracker.cpp).

## PrEP file

`<sim>-PrepOutcomes.xls` reports PrEP prevalence, adherence levels, drop-off, and effective coverage by month. See [`PrepTracker`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/preptracker.cpp).

## CalibStats file

The "did this run pass calibration?" file. For each calibration target (numActs, partnership formation rates, etc.), reports the actual value and whether it falls within the configured `<lwrBound>` / `<uprBound>`.

## Where to add a new column

1. Find the right tracker (`InfectionsTracker`, `PrepTracker`, …).
2. Add a counter, increment it where relevant, value-initialize it (`{}`).
3. In the tracker's `print*` method:
   - Add three header lines (firstRow, secondRow, thirdRow) at month 0
   - Add one body line per row
   - **Be careful with column count** — header and body must have exactly the same number of tabs.
4. Update this page so users know the new column exists.

See the [Developer's Guide](Developers-Guide.md#worked-example-adding-a-new-output-column) for a worked example.

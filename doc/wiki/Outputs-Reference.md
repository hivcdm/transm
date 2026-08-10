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

## Network snapshots

| File | Source | Toggle |
|---|---|---|
| `<sim>-PartnershipNetwork_<month>.graphml` | [`Network::Write`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/partnernetwork.hpp) | `<partnerNetwork enabled="true">`, one file per `<time>` listed |

## End-of-run summary files

| File | Source |
|---|---|
| `<sim>-summaryStats` | `printSummaryStats` |
| `<sim>-IndividualSummaries.json` | `Population::SaveIndividualSummaries` |
| `<sim>-InterventionOutcomes.xls` | `outputs_.intervention_outcomes.Write` |
| `cepacPopstats.out` | CEPAC's RunStats |
| `cepacRunStats` | CEPAC's RunStats |

## `PartnershipNetwork_<month>.graphml`

A snapshot of the whole population and its live partnerships, written for every
month listed under `<traceFiles><partnerNetwork>`. GraphML, undirected, produced
by [`src/statistics/partnernetwork.hpp`](https://github.com/hsphcdm/transm/blob/develop/src/statistics/partnernetwork.hpp)
via Boost.Graph. Opens directly in Gephi; every attribute below shows up as a
column in the Data Laboratory and can be used to partition, size or filter.

Every living entity is a node, including those with no partnership that month
(`is_isolate = 1`). Attribute values are a snapshot of the entity as of the
dump month.

### Node attributes — demographics

| Attribute | Type | Meaning |
|---|---|---|
| `id` | int | entity id, stable across months |
| `gender` | int | 0 male, 1 female |
| `orientation` | int | 0 MSW, 1 MSMW, 2 MSM |
| `race` | int | 0 black, 1 white, 2 other |
| `ethnicity` | int | 0 non-hispanic, 1 hispanic |
| `race_eth` | string | e.g. `:BLACK:NON_HISPANIC` |
| `demo_profile` | string | entity type + race/ethnicity, e.g. `MSM:BLACK:NON_HISPANIC` |
| `is_CSW` | bool | employment is commercial sex work |
| `risk_level` | bool | 0 low, 1 high (assortativeness risk group) |

### Node attributes — HIV and treatment

| Attribute | Type | Meaning |
|---|---|---|
| `hiv_pos` | bool | HIV status is anything other than negative |
| `viral_load` | int | `HVLStrata` ordinal 0-9, 0 if HIV negative |
| `on_prep` | bool | currently on PrEP |
| `on_ART` | bool | currently on ART |
| `hiv_composite` | int | `-1` on PrEP, `0` HIV negative, otherwise `viral_load` |

### Node attributes — care continuum

FOCUS acts by detecting and re-linking people, so these are what make the FOCUS
attributes legible: they show what a selection actually did.

| Attribute | Type | Meaning |
|---|---|---|
| `detected` | bool | detected HIV positive (the PLWH definition) |
| `linked` | bool | linked to care — note this can be true while HIV negative |
| `in_care` | bool | care state is in-care or returned-to-care |
| `ltfu` | bool | currently lost to follow up |
| `rtc` | bool | returned to care after a loss to follow up |
| `care_state` | int | CEPAC `SimContext::HIV_CARE_*`; `-1` if the entity has no CEPAC patient |

### Node attributes — FOCUS

Populated only when the module is on (`--focus on`) and only inside its window,
simulation months `[720, 948)`. With FOCUS off, `focus_selected`/`focus_screened`
are `0`, every int is `-1` or `0`, and `focus_status` is `-1`/`0` throughout. The
values are cumulative to the dump month, not per-month.

| Attribute | Type | Meaning |
|---|---|---|
| `focus_selected` | bool | ever selected by FOCUS |
| `focus_screened` | bool | ever drawn into a FOCUS screening sample, selected or not |
| `focus_month` | int | month of the first selection, `-1` if never |
| `focus_month_last` | int | month of the most recent selection, `-1` if never |
| `months_since_focus` | int | dump month minus `focus_month_last`, `-1` if never |
| `focus_group` | int | `FocusGroup` cohort of the selection, or of the last screening if never selected; `-1` if neither |
| `focus_group_str` | string | that cohort as `RACE:SEX:REASON`, e.g. `BLACK:MALE:LTFU`, or `NONE` |
| `focus_reason` | int | 0 undiagnosed, 1 LTFU, `-1` if never touched |
| `focus_select_count` | int | number of selections |
| `focus_screen_count` | int | number of times sampled |
| `focus_status` | int | composite, see below |

`focus_status` collapses the whole reach-and-yield story into one attribute so it
can be coloured with a single Gephi partition:

| Value | Name | Meaning |
|---|---|---|
| `-1` | not eligible | never met the FOCUS targeting criteria |
| `0` | eligible | eligible but never drawn into a sample |
| `1` | screened | sampled at least once, never won the selection roll |
| `2` | selected (past) | selected more than 12 months before the dump |
| `3` | selected (recent) | selected within 12 months of the dump |

The twelve cohorts and these codes are defined once, in
[`src/entities/focusgroup.hpp`](https://github.com/hsphcdm/transm/blob/develop/src/entities/focusgroup.hpp).
Eligibility for `focus_status` is the reason-and-race condition shared by the
twelve `Population::Find*` predicates: an undiagnosed infection or a loss to
follow up, in one of the three targeted race/ethnicity groups. Note the cohorts
overlap — the `Find*` predicates test race independently, so a Black Hispanic
person is reachable through both the Hispanic and the Black cohort — which is
why `focus_group` records where someone was *drawn from* rather than a canonical
classification.

### Node attributes — structural

| Attribute | Type | Meaning |
|---|---|---|
| `is_isolate` | bool | no partnerships in this snapshot |

Isolates are included deliberately: FOCUS targets undiagnosed and
lost-to-follow-up people, many of whom have no active partnership in a given
month, and building the graph from partnerships alone would drop exactly the
people the FOCUS attributes exist to show. Filter on `is_isolate = 0` in Gephi
if they are in the way of a layout.

### Edge attributes

One edge per live partnership.

| Attribute | Type | Meaning |
|---|---|---|
| `type` | int | `SexualPartnership::Type` — 0 steady, 1 regular, 2 casual, 3 CSW |
| `start` | int | month of formation |
| `end` | int | month of dissolution |
| `duration` | int | `end - start` |
| `genders` | string | entity types joined by `+`, e.g. `MSW+female`, `MSM+MSM` |
| `sero_pos` | int | HIV positive endpoints: 0 neither, 1 discordant, 2 both |
| `focus_edge` | int | endpoints ever selected by FOCUS: 0, 1 or 2 |

`focus_edge` has the same shape as `sero_pos` on purpose — cross the two to ask
whether FOCUS is reaching the serodiscordant edges, the ones that carry
transmission.

### Gotchas

- The dump months come from the XML, but the FOCUS window is fixed in code. A
  `<time>` outside `[720, 948)` gives a file whose FOCUS columns are uniformly
  empty. Pair the `<time>` list with a `<duration>` that reaches it.
- Files are named `<sim>-PartnershipNetwork_<month>.graphml`. Batch runs sharing
  a results directory would otherwise clobber each other.
- A snapshot is the population at that month. Entities that died earlier are
  gone, and ids are not reused, so following an id across two snapshots is valid.

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

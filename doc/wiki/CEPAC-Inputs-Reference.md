# CEPAC Inputs Reference

> **Status: stub.** This page documents the CEPAC `.in` files from transm's perspective — what transm reads, when, and which subset matters for transmission. For complete CEPAC `.in` file documentation, see [`hivcdm/cepac-transm`](https://github.com/hivcdm/cepac-transm).

## What is a CEPAC `.in` file?

A `.in` file is a text-formatted input bundle for CEPAC, the disease-progression model. It defines:

- ART regimens (efficacy, cost, switching policies)
- Disease state transitions (CD4 / HVL dynamics)
- Opportunistic infection rates
- Mortality tables (HIV-related and non-AIDS)
- Care-cascade probabilities (testing, linkage to care, retention)

`transm` references `.in` files by filename in its XML, and CEPAC's `SimContext::readInputs()` parses them into runtime `SimContext` objects.

## Where they live

In MIAMI scenarios, `.in` files live in `MIAMI-prime/INFILES/`. Each scenario uses a small set:

```
INFILES/
├── 07212021B_NoART.in      ← untreated baseline
├── 62M_14_B44m_EXP4.in     ← rollout file for Black males
├── 62M_14_H44m_EXP4.in     ← rollout file for Hispanic males
├── 62M_14_W44n_EXP4.in     ← rollout file for White males (non-Hispanic)
└── ...
```

Naming convention is calibration-specific.

## How they're loaded

For each `.in` file referenced in the XML's `<cepacIntervention>` or `<artRolloutIntervention>` blocks, transm:

1. Constructs a `SimContext(filename_without_extension)` ([`simulation.cpp:945`](https://github.com/hivcdm/transm/blob/develop/src/core/simulation.cpp#L945))
2. Calls `context->readInputs()` to parse the file into the `SimContext` object
3. Stores the resulting `SimContext*` in either `cepacSimContexts` (non-rollout) or `rolloutSimContexts` (rollout)

This is detailed in [CEPAC Integration](Mechanism-CEPAC-Integration.md#configuration-time--loading-cepac-in-files).

## What transm reads from `.in` files

CEPAC owns most of the `.in` data — it's only consumed inside CEPAC's clinical engine. Transm itself reads only:

| Section in `.in` | Consumed by | Used for |
|---|---|---|
| Non-AIDS death probabilities (male & female) | [`CepacInputParser`](https://github.com/hivcdm/transm/blob/develop/src/utility/cepacinputparser.cpp) | Background mortality table for HIV-negative entities |
| Discount factor | `Utility::computeCepacDiscountFactor` | Cost discounting in CE-related outputs |
| `runSpecsInputs->discountFactor` | rollout context | Cost calculations |
| Everything else (CD4/HVL transitions, ART, OIs, ...) | CEPAC, internally | Per-patient disease state, mortality |

So if you want to change disease progression, edit the `.in` file. If you want to change transmission probability, edit transm's XML — the `.in` file has nothing to do with transmission.

## `CepacInputParser`

`transm/src/utility/cepacinputparser.{cpp,hpp}` is the only place transm parses `.in` content directly. It reads tab-delimited tables for non-AIDS death probabilities. Look there for a worked example of the file format.

## Common pitfalls

1. **Editing the wrong file.** If your transmission rates aren't matching expectation, the `.in` file is almost never the culprit — transmission lives in transm's XML.
2. **Mismatched calibration anchors.** The CEPAC `.in` file has its own time origin (typically simulating individuals from age 0 with no notion of calendar time); transm's `<monthOf1990>` is what aligns simulation time to real years. They have to be consistent for cost discounting and time-anchored interventions.
3. **Numeric format.** CEPAC's parser expects exact whitespace — copy-paste from a spreadsheet often inserts smart quotes or non-breaking spaces. If `readInputs()` throws, suspect formatting.

## Reference

For a full description of every `.in` file section, see the CEPAC repository's documentation. This wiki page intentionally stays brief because that's the right place for it; transm only consumes a sliver.

# Model checks

Small runs of the calibrated Miami scenario that check the model still does its
job. They do not ask for identical outputs, so a changed method may change
results as long as these still hold.

```
ctest -L model                                   # from the build directory, ~3 minutes on 4 cores
python3 tests/model/model_checks.py --transm bin/transm-v4.8.0
python3 tests/model/model_checks.py --transm ... --only stress --work /tmp/runs   # keep the runs
```

## What is checked

- **run**: every run exits cleanly and writes its tables.
- **bookkeeping**: rules every month of every run must satisfy, such as
  population = last month + births − deaths, or diagnosed + undiagnosed = infected.
- **known answers**: outcomes known without running, such as nobody being infected
  without seeding, or no new infections when transmission is zero.
- **bands**: about 20 indicators (new infections, deaths, people on ART, PrEP
  users, FOCUS selections) averaged over 4 seeds must lie within 4.5 standard
  errors of the 30 reference seeds in `reference.json`.

The benchmarks are `scenarios/miami_2k.xml` (2,000 people, all 840 months, so
ART rollout, seeding, PrEP and FOCUS all happen) with the edits listed in
`BENCHMARKS` in `model_checks.py`. The `stress` edits seed more infections and
raise transmission and PrEP access, because the calibrated scenario has only
about six new infections at this size.

## When results are meant to change

A change that is meant to alter the epidemic, such as a new method or a bug fix,
will move some bands. Check that the new values make sense, then record them
again and commit `reference.json` with the change:

```
python3 tests/model/model_checks.py --transm bin/transm-v4.8.0 --record
```

This takes about 15 minutes. A refactoring or speedup should not move anything;
for those, `tests/regression/compare_binaries.sh` requires byte-identical
outputs from the old and new builds.

## What these checks cannot see

Effects too small to stand out at 2,000 people. For example, a build in which
PrEP gave no protection passed every check here, because only about 60 people
use PrEP. Effects like that are covered by unit tests of the method itself, such
as `tests/InfectionRiskTest.cpp` for the force of infection.

## Known issues

These are reported as `KNOWN ISSUE` and do not fail the run; `--strict` makes
them fail.

- CEPAC crashes at exit (`RunStats::finalizeSurvivalStats`) in about half the
  stress runs, after the tables are written. Valgrind traces uninitialised
  values to the `Patient` objects CEPAC creates for each person. Fixed in
  cepac-transm PR #10; once it is merged, remove the allowance in
  `crashed_at_exit` so a crash fails the checks again.

`INFILES/` holds the CEPAC inputs the scenario uses. They keep their CRLF line
endings (see `.gitattributes`).

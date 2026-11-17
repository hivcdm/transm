#!/usr/bin/env python3
"""Run small TRANSM scenarios and check what must hold however the model's methods
are implemented. These checks do not ask for identical outputs; a change to a
method is allowed to change results, as long as the model still does its job.

Each benchmark is scenarios/miami_2k.xml (the calibrated Miami PrEP scenario at
2,000 people, all 840 months, so ART rollout, seeding, PrEP and FOCUS all happen)
with a few edits listed in BENCHMARKS below. Three kinds of check:

  run          the run exits cleanly and writes its tables
  bookkeeping  rules every month of every run must satisfy, e.g. population this
               month = last month + births - deaths
  known answer outcomes we know without running, e.g. no seeded infections means
               nobody is ever infected
  bands        indicators (infections, deaths, people on ART, PrEP users, ...) over
               several seeds must sit inside bands recorded from reference runs in
               reference.json. A change that is meant to alter results records new
               bands with --record and commits reference.json with it, the same
               way a changed method comes with a changed unit test.

Usage:
  model_checks.py --transm BIN [--only NAME ...] [--jobs N] [--work DIR]
  model_checks.py --transm BIN --record          # rewrite reference.json

Exit status: 0 all checks pass, 1 a check failed, 2 bad usage.
"""

import argparse
import concurrent.futures
import copy
import json
import math
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from transm_outputs import Table  # noqa: E402

BASE_SCENARIO = os.path.join(HERE, "scenarios", "miami_2k.xml")
INFILES = os.path.join(HERE, "INFILES")
REFERENCE = os.path.join(HERE, "reference.json")

# Calendar of the base scenario, in simulation months.
SEED_MONTH = 602        # <initialInfections><delay>
PREP_START_MONTH = 684  # PrEP starts in simulation year 57 (Entity::updatePrepStatus)
FOCUS_FIRST_MONTH = 720  # FOCUS screens from month 720 (Simulation::Step)
DURATION = 840          # <duration>


# ---------------------------------------------------------------- scenario edits

def scale_transmission(factor):
    def edit(root):
        for coefficients in root.iter("transmissionCoefficients"):
            for e in coefficients.iter():
                if len(e) == 0 and e.text and e.text.strip():
                    e.text = repr(float(e.text) * factor)
    return edit


def set_seed_prevalence(value):
    def edit(root):
        for e in root.iter("seedPrevalence"):
            e.text = repr(value)
    return edit


def scale_prep_access(factor):
    """Multiply the monthly PrEP access and return-to-care rates of every profile."""
    def edit(root):
        for tag in ("prepAccess", "prepReturnToCare"):
            for e in root.iter(tag):
                e.text = ",".join(repr(min(1.0, float(v) * factor)) for v in e.text.split(","))
    return edit


def remove_prep(root):
    population = root.find("population")
    population.remove(population.find("preExposureProphylaxis"))


# More seeded infections and stronger transmission than the calibrated scenario,
# whose 2,000 people see only about six new infections in 840 months. This gives
# the infection, testing, ART and PrEP paths enough events to check. PrEP access
# is raised too: at the calibrated rates only a handful of the 2,000 ever start.
STRESS = [set_seed_prevalence(0.1), scale_transmission(4.0), scale_prep_access(50.0)]

BENCHMARKS = {
    # name: edits, --focus, seeds for the bands (0 = single run, no bands)
    "calibrated": dict(edits=[], focus=False, seeds=0),
    "stress": dict(edits=STRESS, focus=False, seeds=4),
    "stress_focus": dict(edits=STRESS, focus=True, seeds=4),
    "stress_no_prep": dict(edits=STRESS + [remove_prep], focus=False, seeds=0),
    "no_transmission": dict(edits=[set_seed_prevalence(0.1), scale_transmission(0.0)], focus=False, seeds=0),
    "no_seeding": dict(edits=[set_seed_prevalence(0.0)], focus=False, seeds=0),
}
# Seeds of the reference runs behind reference.json. The checks run the first
# few of them, so an unchanged model reproduces its reference runs exactly.
REFERENCE_SEEDS = list(range(1, 31))


# ---------------------------------------------------------------- running

class Run:
    def __init__(self, benchmark, seed, directory):
        self.benchmark = benchmark
        self.seed = seed
        self.directory = directory
        self.status = None
        self.seconds = None
        self.indicators = None
        self._tables = {}

    @property
    def label(self):
        return f"{self.benchmark} seed {self.seed}"

    def release_tables(self):
        self._tables.clear()

    def table(self, name):
        if name not in self._tables:
            self._tables[name] = Table(os.path.join(self.directory, "results_run", f"run-{name}.xls"))
        return self._tables[name]


def write_scenario(benchmark, seed, path):
    tree = ET.parse(BASE_SCENARIO)
    root = tree.getroot()
    for edit in BENCHMARKS[benchmark]["edits"]:
        edit(root)
    root.find("fixedSeed").text = str(seed)
    tree.write(path, encoding="utf-8", xml_declaration=True)


def execute(transm, run, timeout):
    if os.path.exists(run.directory):
        shutil.rmtree(run.directory)
    os.makedirs(run.directory)
    write_scenario(run.benchmark, run.seed, os.path.join(run.directory, "run.xml"))
    os.symlink(INFILES, os.path.join(run.directory, "INFILES"))
    focus = "on" if BENCHMARKS[run.benchmark]["focus"] else "off"
    start = time.monotonic()
    with open(os.path.join(run.directory, "run.log"), "w") as log:
        try:
            run.status = subprocess.run([transm, "--cepac", "INFILES", "--focus", focus, "run.xml"],
                                        cwd=run.directory, stdout=log, stderr=subprocess.STDOUT,
                                        timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            run.status = "timeout"
    run.seconds = time.monotonic() - start
    with open(os.path.join(run.directory, "exit_status"), "w") as f:
        f.write(f"{run.status}\n")
    return run


# ---------------------------------------------------------------- checks

# Checks that fail on purpose today, with the reason. They are reported but do not
# fail the run; once one passes, it is reported so the entry can be removed.
KNOWN_ISSUES = {}


class Checks:
    def __init__(self):
        self.failures = []
        self.known = []
        self.passed = 0

    def expect(self, ok, what, known=None):
        if known:
            if ok:
                self.known.append(f"{what}: now passes; remove '{known}' from KNOWN_ISSUES")
            else:
                self.known.append(f"{what} ({KNOWN_ISSUES[known]})")
            return ok
        if ok:
            self.passed += 1
        else:
            self.failures.append(what)
        return ok


def first_bad_month(table, values):
    """Month label of the first False in values, or None."""
    for month, ok in zip(table.months, values):
        if not ok:
            return month
    return None


def check_rule(checks, run, table, name, values):
    month = first_bad_month(table, values)
    checks.expect(month is None, f"{run.label}: {name} fails at month {month}")


def bookkeeping(checks, run):
    pop_t = run.table("Population")
    inf_t = run.table("Infections")
    art_t = run.table("ARTRollout")
    prep_t = run.table("PrepOutcomes")

    pop = pop_t.column("Pop Size")
    births = pop_t.column("Births")
    deaths = pop_t.column("Deaths", "Total")
    causes = [pop_t.column("Deaths", c) for c in
              ("Any OI", "Chronic AIDS", "Non-AIDS", "ART Toxicity", "Proph Toxicity", "Other")]
    females = pop_t.column("Gender", "Females")
    males = pop_t.column("Gender", "All Males")
    orientations = [pop_t.column("Male By Orientation", o) for o in ("MSW", "MSMW", "MSM")]
    races = [pop_t.column(*c.split(" / ")) for c in pop_t.columns if c.startswith("Race and Ethnicity /")]
    sa = pop_t.column("SA Pop Size")
    non_sa = pop_t.column("Non-SA Pop (All Ages)", "All ages")
    months = range(len(pop))

    check_rule(checks, run, pop_t, "population = last month + births - deaths",
               [True] + [pop[i] == pop[i - 1] + births[i] - deaths[i] for i in months if i > 0])
    check_rule(checks, run, pop_t, "deaths by cause add up to total deaths",
               [sum(c[i] for c in causes) == deaths[i] for i in months])
    check_rule(checks, run, pop_t, "females + males = population",
               [females[i] + males[i] == pop[i] for i in months])
    check_rule(checks, run, pop_t, "MSW + MSMW + MSM = males",
               [sum(o[i] for o in orientations) == males[i] for i in months])
    check_rule(checks, run, pop_t, "race/ethnicity groups add up to population",
               [sum(r[i] for r in races) == pop[i] for i in months])
    check_rule(checks, run, pop_t, "sexually active + not sexually active = population",
               [sa[i] + non_sa[i] == pop[i] for i in months])

    infected = inf_t.column("Currently Infected")
    new = inf_t.column("New Infections")
    history = inf_t.column("Epidemiology Outputs", "Total Infected in History (Prevalent Cases Excluded)")
    prevalence = inf_t.column("Epidemiology Outputs", "Prevalence")
    inf_pop = inf_t.column("Epidemiology Outputs", "Pop Size")
    by_gender = [inf_t.column("Prevalent Cases", "By Gender", g) for g in ("MALE", "FEMALE")]

    check_rule(checks, run, inf_t, "Infections and Population tables agree on population",
               [inf_pop[i] == pop[i] for i in months])
    check_rule(checks, run, inf_t, "0 <= infected <= population",
               [0 <= infected[i] <= pop[i] for i in months])
    check_rule(checks, run, inf_t, "prevalence = infected / population",
               [abs(prevalence[i] - infected[i] / pop[i]) < 1e-6 for i in months])
    check_rule(checks, run, inf_t, "infected men + infected women = infected",
               [by_gender[0][i] + by_gender[1][i] == infected[i] for i in months])
    check_rule(checks, run, inf_t, "infected-in-history grows by exactly the new infections",
               [True] + [history[i] - history[i - 1] == new[i] for i in months if i > 0])

    art_infected = gender_total(art_t, "Number Infected")
    detected = gender_total(art_t, "Number Detected")
    undiagnosed = gender_total(art_t, "Undiagnosed")
    in_care = gender_total(art_t, "Number in Care")
    on_art = gender_total(art_t, "On ART")
    suppressed = gender_total(art_t, "Suppressed VL")
    # The ART table's row for month m shows the state the Infections table
    # reports for month m - 1: it is recorded before the month's infections.
    check_rule(checks, run, art_t, "ART table's infected = Infections table's infected a month earlier",
               [True] + [art_infected[i] == infected[i - 1] for i in months if i > 0])
    check_rule(checks, run, art_t, "diagnosed + undiagnosed = infected",
               [detected[i] + undiagnosed[i] == art_infected[i] for i in months])
    check_rule(checks, run, art_t, "in care <= diagnosed",
               [in_care[i] <= detected[i] for i in months])
    check_rule(checks, run, art_t, "on ART <= in care",
               [on_art[i] <= in_care[i] for i in months])
    check_rule(checks, run, art_t, "virally suppressed <= infected",
               [suppressed[i] <= art_infected[i] for i in months])

    users = gender_total(prep_t, "Number of Prep Users")
    eligible = gender_total(prep_t, "Number Eligible for Prep")
    adherent = gender_total(prep_t, "Number of Adherent")
    check_rule(checks, run, prep_t, "PrEP users <= people eligible for PrEP",
               [users[i] <= eligible[i] for i in months])
    check_rule(checks, run, prep_t, "adherent PrEP users <= PrEP users",
               [adherent[i] <= users[i] for i in months])
    check_rule(checks, run, prep_t, "no PrEP before simulation year 57",
               [users[i] == 0 and eligible[i] == 0 for i in months if i < PREP_START_MONTH])


def gender_total(table, group):
    """Sum of a group's men and women columns, for groups without a total column."""
    cols = [c for c in table.columns if c.startswith(group + " / Gender /")]
    if len(cols) != 2:
        raise KeyError(f"{table.path}: expected Males and Females under '{group} / Gender', found {cols}")
    return [a + b for a, b in zip(table.column(*cols[0].split(" / ")), table.column(*cols[1].split(" / ")))]


def known_answers(checks, run):
    inf_t = run.table("Infections")
    infected = inf_t.column("Currently Infected")
    new = inf_t.column("New Infections")
    users = gender_total(run.table("PrepOutcomes"), "Number of Prep Users")
    deaths = run.table("Population")
    hiv_deaths = [a + b for a, b in zip(deaths.column("Deaths", "Any OI"), deaths.column("Deaths", "Chronic AIDS"))]
    name = run.benchmark

    if name != "no_seeding":
        checks.expect(all(v == 0 for v in infected[:SEED_MONTH]) and infected[SEED_MONTH] > 0,
                      f"{run.label}: infections should first appear at the seeding month {SEED_MONTH}")
    if name == "no_seeding":
        checks.expect(all(v == 0 for v in infected), f"{run.label}: nobody should be infected without seeding")
        checks.expect(all(v == 0 for v in hiv_deaths), f"{run.label}: no HIV deaths without infections")
        # PrEP users are still possible: a high-risk partner or a recent condomless
        # act is enough to be eligible (<prepEligibility>, any one risk criterion).
    if name == "no_transmission":
        checks.expect(all(v == 0 for v in new), f"{run.label}: no new infections when transmission is zero")
        later = infected[SEED_MONTH:]
        checks.expect(all(b <= a for a, b in zip(later, later[1:])),
                      f"{run.label}: infected count can only fall after seeding when nobody transmits")
    if name == "stress_no_prep":
        checks.expect(all(v == 0 for v in users), f"{run.label}: no PrEP users when PrEP is not configured")
    if name.startswith("stress") and name != "stress_no_prep":
        checks.expect(max(users[PREP_START_MONTH:]) > 0, f"{run.label}: someone should start PrEP")
    if name.startswith("stress"):
        checks.expect(sum(new) > 50, f"{run.label}: stress scenario should have many new infections, has {sum(new):.0f}")
    focus = focus_selected(run)
    if BENCHMARKS[name]["focus"]:
        checks.expect(sum(focus[FOCUS_FIRST_MONTH:]) > 0, f"{run.label}: FOCUS should select people for screening")
        checks.expect(sum(focus[:FOCUS_FIRST_MONTH]) == 0, f"{run.label}: FOCUS should not screen before month {FOCUS_FIRST_MONTH}")
    else:
        checks.expect(sum(focus) == 0, f"{run.label}: FOCUS should screen nobody with --focus off")


def focus_selected(run):
    return gender_total(run.table("ARTRollout"), "Selected for FOCUS Study")


# ---------------------------------------------------------------- bands

def indicators(run):
    """The numbers the bands are kept for, from one run."""
    pop_t = run.table("Population")
    inf_t = run.table("Infections")
    new = inf_t.column("New Infections")
    deaths = pop_t.column("Deaths", "Total")
    hiv_deaths = [a + b for a, b in zip(pop_t.column("Deaths", "Any OI"), pop_t.column("Deaths", "Chronic AIDS"))]
    on_art = gender_total(run.table("ARTRollout"), "On ART")
    users = gender_total(run.table("PrepOutcomes"), "Number of Prep Users")
    last = len(new) - 1
    values = {
        "new infections, seeding to 1990s (months 603-720)": sum(new[SEED_MONTH + 1:721]),
        "new infections, months 721-840": sum(new[721:]),
        "infected at month 840": inf_t.column("Currently Infected")[last],
        "population at month 840": pop_t.column("Pop Size")[last],
        "sexually active at month 840": pop_t.column("SA Pop Size")[last],
        "deaths, all causes": sum(deaths[1:]),
        "HIV deaths (OI and chronic AIDS)": sum(hiv_deaths[1:]),
        "on ART at month 840": on_art[last],
        "PrEP users, mean of months 720-840": statistics.fmean(users[720:]),
    }
    if BENCHMARKS[run.benchmark]["focus"]:
        values["selected by FOCUS"] = sum(focus_selected(run))
    return values


def band_limit(reference_values, candidates):
    """How far the candidates' mean may sit from the reference mean.

    4.5 standard errors of the difference of two means. With 30 reference seeds a
    model that only draws different random numbers fails one indicator about once in
    10,000 (Student's t, 29 degrees of freedom), so about 0.2% of runs over the ~20
    indicators. One person is added so that indicators that hardly vary across seeds
    do not fail on a single event."""
    sd = statistics.stdev(reference_values)
    return 4.5 * sd * math.sqrt(1 / len(reference_values) + 1 / candidates) + 1


def bands(checks, benchmark, runs, reference):
    if benchmark not in reference:
        checks.expect(False, f"{benchmark}: no bands in reference.json; record them with --record")
        return
    measured = [r.indicators for r in runs]
    for name, ref_values in reference[benchmark].items():
        got = [m[name] for m in measured]
        mean_ref, mean_got = statistics.fmean(ref_values), statistics.fmean(got)
        limit = band_limit(ref_values, len(got))
        checks.expect(abs(mean_got - mean_ref) <= limit,
                      f"{benchmark}: {name} averages {mean_got:.1f} over {len(got)} seeds, "
                      f"reference {mean_ref:.1f} +/- {limit:.1f}")


# ---------------------------------------------------------------- main

def load_reference():
    if not os.path.exists(REFERENCE):
        return {}
    with open(REFERENCE) as f:
        return json.load(f)


def crashed_at_exit(run):
    """True when a run was killed by a signal after its last month, with its TRANSM
    tables written. CEPAC's RunStats::finalizeSurvivalStats crashes there (it sorts
    patient summaries whose life months can be uninitialised), losing run.out but
    none of the tables these checks read."""
    if not isinstance(run.status, int) or run.status >= 0:
        return False
    try:
        return run.table("Population").months[-1] == str(DURATION)
    except (OSError, ValueError):
        return False


def plan(names, record, work, first_seed):
    planned = []
    for name in names:
        banded = BENCHMARKS[name]["seeds"]
        if record:
            chosen = REFERENCE_SEEDS if banded else []
        else:
            chosen = list(range(first_seed, first_seed + banded)) if banded else [0]
        planned += [Run(name, seed, os.path.join(work, f"{name}-seed{seed}")) for seed in chosen]
    return planned


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--transm", help="TRANSM binary to check (not needed with --no-run)")
    parser.add_argument("--only", nargs="+", choices=sorted(BENCHMARKS), help="run only these benchmarks")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1), help="runs at the same time")
    parser.add_argument("--work", help="directory for the runs (kept); default: a temporary one, removed on success")
    parser.add_argument("--timeout", type=int, default=1800, help="seconds allowed per run")
    parser.add_argument("--record", action="store_true",
                        help=f"run seeds {REFERENCE_SEEDS[0]}-{REFERENCE_SEEDS[-1]} and rewrite reference.json")
    parser.add_argument("--no-run", action="store_true", help="check the runs already in --work again")
    parser.add_argument("--first-seed", type=int, default=REFERENCE_SEEDS[0],
                        help="first seed of the banded runs; seeds the reference never used show "
                             "whether the bands tolerate a change that only reshuffles random draws")
    parser.add_argument("--strict", action="store_true",
                        help="also fail on CEPAC's known crash at exit (see crashed_at_exit)")
    args = parser.parse_args()

    if args.no_run and not args.work:
        parser.error("--no-run needs --work")
    if not args.no_run:
        if not args.transm:
            parser.error("--transm is required")
        transm = os.path.abspath(args.transm)
        if not os.access(transm, os.X_OK):
            parser.error(f"{transm} is not an executable")
    work = os.path.abspath(args.work) if args.work else tempfile.mkdtemp(prefix="transm-model-checks.")
    names = args.only or list(BENCHMARKS)
    planned = plan(names, args.record, work, args.first_seed)
    if not planned:
        parser.error("none of the chosen benchmarks keeps bands")

    if args.no_run:
        for run in planned:
            with open(os.path.join(run.directory, "exit_status")) as f:
                status = f.read().strip()
            run.status = int(status) if status.lstrip("-").isdigit() else status
    else:
        print(f"{len(planned)} runs of {transm}, {args.jobs} at a time, in {work}", flush=True)
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            for run in pool.map(lambda r: execute(transm, r, args.timeout), planned):
                print(f"  {run.label}: exit {run.status} in {run.seconds:.0f} s", flush=True)

    checks = Checks()
    known = checks.known
    finished = []
    for run in planned:
        if run.status == 0:
            checks.expect(True, "")
            finished.append(run)
        elif crashed_at_exit(run) and not args.strict:
            known.append(f"{run.label}: crashed with signal {-run.status} after its last month "
                         f"(CEPAC's crash at exit); its tables were checked, run.out was lost")
            finished.append(run)
        else:
            checks.expect(False, f"{run.label}: exited with {run.status}, see {run.directory}/run.log")

    for run in finished:
        try:
            bookkeeping(checks, run)
            if not args.record:
                known_answers(checks, run)
            if BENCHMARKS[run.benchmark]["seeds"]:
                run.indicators = indicators(run)
        except (OSError, ValueError, KeyError) as e:
            checks.expect(False, f"{run.label}: could not read outputs: {e}")
        # Parsed tables take tens of MB per run; keep only the indicators.
        run.release_tables()

    reference = load_reference()
    if args.record:
        recorded = []
        for name in names:
            runs = [r for r in finished if r.benchmark == name]
            if BENCHMARKS[name]["seeds"] and len(runs) == len(REFERENCE_SEEDS):
                per_run = [r.indicators for r in runs]
                reference[name] = {k: [m[k] for m in per_run] for k in per_run[0]}
                recorded.append(name)
        if checks.failures:
            print("Not recording: the reference runs fail their own checks.")
        else:
            with open(REFERENCE, "w") as f:
                json.dump(reference, f, indent=1, sort_keys=True)
                f.write("\n")
            print(f"Recorded bands for {', '.join(recorded)} in {REFERENCE}")
    else:
        for name in names:
            runs = [r for r in finished if r.benchmark == name]
            if BENCHMARKS[name]["seeds"]:
                if len(runs) == BENCHMARKS[name]["seeds"]:
                    bands(checks, name, runs, reference)
                else:
                    checks.expect(False, f"{name}: bands need all {BENCHMARKS[name]['seeds']} runs to finish")

    for issue in known:
        print("KNOWN ISSUE:", issue)
    for failure in checks.failures:
        print("FAIL:", failure)
    if checks.failures:
        print(f"{len(checks.failures)} of {checks.passed + len(checks.failures)} checks failed; runs kept in {work}")
        return 1
    print(f"PASS: {checks.passed} checks" + (f", {len(known)} known issues" if known else ""))
    if not args.work:
        shutil.rmtree(work)
    return 0


if __name__ == "__main__":
    sys.exit(main())

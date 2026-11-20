#!/usr/bin/env python3
"""Find TRANSM code that is never used.

Two questions, two answers:

  Can this function ever be called?
      Every function is compiled into its own section and the linker is asked to
      drop the sections nothing reachable from main() refers to. What it drops
      cannot run, whatever the scenario.

  Is this code called by the scenarios we run?
      A second binary counts every line it executes. Run it on the scenarios
      that matter; whatever stays at zero is not used by them.

Usage:
  unused_code.py build OUT [--build-dir DIR] [--jobs N]
      Compile both binaries into OUT. The normal build is left alone, but it
      must exist: its third-party sources and lib/*.a are reused.
      Writes OUT/unreachable.tsv and OUT/cov/transm.

  OUT/cov/transm --cepac INFILES --focus on run.xml      # as often as you like
      Counts add up over runs. It is several times slower than a release build.

  unused_code.py report OUT
      List the functions and lines no run has reached. Functions marked R are
      reachable, so another scenario could still call them; the rest are also
      in unreachable.tsv.

Limits: a virtual function counts as reachable as soon as its class is used, and
a function only called from the unit tests is reported as unreachable. Removing
dead code can leave its callees dead in turn, so run this again afterwards.
"""

import argparse
import collections
import concurrent.futures
import glob
import json
import os
import re
import signal
import subprocess
import sys

REPO = os.path.realpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SRC = os.path.join(REPO, "src")
COMMON = ["-fpermissive", "-fopenmp", "-std=gnu++14", "-DUSE_GOOGLE_PERF_TOOLS=OFF", "-O0", "-g1",
          "-fno-inline", "-fkeep-inline-functions"]   # keep functions defined in headers, used or not
REACH = ["-ffunction-sections", "-fdata-sections"]
COVERAGE = ["--coverage"]

# CEPAC can crash at exit, which would lose the counters of that run.
DUMP_ON_CRASH = """
#include <csignal>
extern "C" void __gcov_dump(void);
static void dump_and_die(int sig) { __gcov_dump(); signal(sig, SIG_DFL); raise(sig); }
static struct Install { Install() { signal(SIGSEGV, dump_and_die); signal(SIGABRT, dump_and_die); } } install;
"""


def sources():
    with open(os.path.join(REPO, "cmake.sources")) as listing:
        return [line.strip() for line in listing if line.strip().endswith(".cpp")]


def compile_all(out, flags, build_dir, jobs):
    third_party = os.path.join(build_dir, "third-party", "src")
    includes = ["-I" + os.path.join(third_party, d) for d in
                ("rana_project/include", "tclap_project/include", "pugixml_project/src", "cepac_project/src")]
    includes.append("-I" + SRC)
    os.makedirs(out, exist_ok=True)

    def compile_one(source):
        obj = os.path.join(out, source.replace("/", "_")[:-len(".cpp")] + ".o")
        done = subprocess.run(["g++", *COMMON, *flags, *includes, "-c", os.path.join(REPO, source), "-o", obj],
                              capture_output=True, text=True)
        if done.returncode != 0:
            sys.exit(f"could not compile {source}:\n{done.stderr[-3000:]}")
        return obj

    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        return list(pool.map(compile_one, sources()))


def link(out, objects, flags):
    libs = [os.path.join(REPO, "lib", "libcepac.a"), os.path.join(REPO, "lib", "libpugixml.a"),
            "-lsqlite3", "-lquadmath"]
    done = subprocess.run(["g++", "-fopenmp", *flags, *objects, *libs, "-o", os.path.join(out, "transm")],
                          capture_output=True, text=True)
    if done.returncode != 0:
        sys.exit(f"could not link {out}/transm:\n{done.stderr[-3000:]}")


def function_symbols(binary, with_location=False):
    """Functions defined in an object or binary: {symbol: 'file:line' or ''}."""
    command = ["nm", "--defined-only"] + (["-l"] if with_location else []) + [binary]
    found = {}
    for line in subprocess.run(command, capture_output=True, text=True).stdout.splitlines():
        fields = line.split(None, 2)
        if len(fields) == 3 and fields[1] in "TtWw":
            symbol, _, location = fields[2].partition("\t")
            found.setdefault(symbol, location)
    return found


def build_reachability(out, build_dir, jobs):
    print("compiling for reachability ...", flush=True)
    objects = compile_all(os.path.join(out, "reach"), REACH, build_dir, jobs)
    link(os.path.join(out, "reach"), objects, ["-Wl,--gc-sections"])
    list_unreachable(out, objects, jobs)


def list_unreachable(out, objects, jobs):
    """Functions defined in src/ whose section the linker dropped."""
    linked = function_symbols(os.path.join(out, "reach", "transm"))

    defined = {}
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        for symbols in pool.map(lambda obj: function_symbols(obj, with_location=True), objects):
            for symbol, location in symbols.items():
                if os.path.realpath(location.rsplit(":", 1)[0]).startswith(SRC + os.sep):
                    defined.setdefault(symbol, os.path.relpath(os.path.realpath(location.rsplit(":", 1)[0]), SRC)
                                       + ":" + location.rsplit(":", 1)[1])
    mangled = sorted(defined)
    names = subprocess.run(["c++filt"], input="\n".join(mangled), capture_output=True, text=True).stdout.splitlines()

    # A constructor or destructor has several symbols under one name; it is dead only if all are.
    by_name = collections.defaultdict(list)
    for symbol, name in zip(mangled, names):
        by_name[name].append(symbol)
    dead = [(defined[symbols[0]], name, symbols) for name, symbols in by_name.items()
            if not any(symbol in linked for symbol in symbols)]
    dead.sort(key=lambda row: (row[0].rsplit(":", 1)[0], int(row[0].rsplit(":", 1)[1])))

    with open(os.path.join(out, "unreachable.tsv"), "w") as table:
        for location, name, symbols in dead:
            table.write(f"{location}\t{name}\t{' '.join(symbols)}\n")
    print(f"{len(by_name)} functions in src/, {len(dead)} unreachable from main(): {out}/unreachable.tsv")


def build_coverage(out, build_dir, jobs):
    print("compiling for coverage ...", flush=True)
    cov = os.path.join(out, "cov")
    objects = compile_all(cov, COVERAGE, build_dir, jobs)
    helper = os.path.join(cov, "dump_on_crash.cpp")
    with open(helper, "w") as f:
        f.write(DUMP_ON_CRASH)
    subprocess.run(["g++", "-c", helper, "-o", helper[:-len(".cpp")] + ".o"], check=True)
    link(cov, objects + [helper[:-len(".cpp")] + ".o"], COVERAGE)
    print(f"now run {cov}/transm on your scenarios, then: unused_code.py report {out}")


def report(out):
    cov = os.path.join(out, "cov")
    unreachable = set()
    if os.path.exists(os.path.join(out, "unreachable.tsv")):
        with open(os.path.join(out, "unreachable.tsv")) as table:
            unreachable = {symbol for line in table for symbol in line.rstrip("\n").split("\t")[2].split()}

    calls = {}                               # (file, first line, name, symbol) -> times called
    lines = collections.defaultdict(dict)    # file -> line -> times run
    counters = sorted(glob.glob(os.path.join(cov, "*.gcda")))
    if not counters:
        sys.exit(f"no counters in {cov}: run {cov}/transm on a scenario first")
    for gcda in counters:
        gcov = subprocess.run(["gcov", "--json-format", "--stdout", "--demangled-names", gcda],
                              capture_output=True, text=True, cwd=cov).stdout
        for document in gcov.splitlines():
            if not document.startswith("{"):
                continue
            for entry in json.loads(document)["files"]:
                path = os.path.realpath(os.path.join(cov, entry["file"]))
                if not path.startswith(SRC + os.sep):
                    continue
                source = os.path.relpath(path, SRC)
                for function in entry["functions"]:
                    key = (source, function["start_line"], function["demangled_name"], function["name"])
                    calls[key] = calls.get(key, 0) + function["execution_count"]
                for line in entry["lines"]:
                    number = line["line_number"]
                    lines[source][number] = lines[source].get(number, 0) + line["count"]

    never = sorted(key for key, count in calls.items() if count == 0)
    print(f"{len(calls)} functions in src/, {len(never)} never ran\n")
    print("lines run / lines of code")
    for source in sorted(lines):
        ran = sum(1 for count in lines[source].values() if count)
        print(f"  {ran:5d} / {len(lines[source]):5d}  {100 * ran // max(1, len(lines[source])):3d}%  {source}")
    print("\nnever ran (R = reachable from main, so another scenario could call it)")
    for source, line, name, symbol in never:
        # Lambdas and compiler-made copies carry their parent's story; keep the list readable.
        if re.search(r"\{lambda|::_FUN", name):
            continue
        print(f"  {'-' if symbol in unreachable else 'R'}  {source}:{line}  {name}")


def main():
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)   # allow "report OUT | head"
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("action", choices=["build", "report"])
    parser.add_argument("out", help="directory for the two builds and their results")
    parser.add_argument("--build-dir", default=os.path.join(REPO, "build"), help="existing CMake build directory")
    parser.add_argument("--jobs", type=int, default=3, help="compilers at the same time (each needs about 1 GB)")
    args = parser.parse_args()
    out = os.path.abspath(args.out)

    if args.action == "build":
        if not os.path.isdir(os.path.join(args.build_dir, "third-party", "src", "cepac_project")):
            sys.exit(f"{args.build_dir} is not a finished TRANSM build; run ./build.sh first")
        build_reachability(out, args.build_dir, args.jobs)
        build_coverage(out, args.build_dir, args.jobs)
    else:
        report(out)


if __name__ == "__main__":
    main()

#!/bin/bash
#
# Run the same scenario with two TRANSM binaries and require byte-identical
# outputs. Use it to show that a refactoring or speedup leaves results unchanged:
# build the reference binary from the base branch, the candidate from the
# working branch, and compare them under a fixed seed.
#
# Usage:
#   compare_binaries.sh REF_BIN NEW_BIN SCENARIO_XML CEPAC_DIR [SIZE] [DURATION]
#
#   SIZE, DURATION  optional overrides for the initial population <size> and the
#                   run <duration> in months, so a large scenario can be shrunk
#                   to a quick check. The scenario must use a fixed seed
#                   (<fixedSeed> >= 0).
#
# Environment:
#   SEED            optional override for <fixedSeed> (must be >= 0).
#   WORK_DIR        where the two runs are written (default: a new temp dir).
#                   It is kept after the run so differences can be inspected.
#
# TRANSM outputs must be byte-identical. The two CEPAC summaries (run.out and
# cepacPopstats.out) are compared with their timestamps masked, and a difference
# there is only a warning: two runs of the same develop binary already differ in
# the "Secondary OIs" tables, which points to uninitialised memory in CEPAC.
#
# Exit status:
#   0  every output matches and both runs exited cleanly
#   1  a TRANSM output differs, or the runs wrote different sets of files
#   3  TRANSM outputs match, but a CEPAC summary differs or a run exited with an
#      error (develop has an intermittent crash at exit, after outputs are written)

set -u

if [[ $# -lt 4 ]]; then
    awk 'NR > 2 && /^#/ { print; next } NR > 2 { exit }' "$0"
    exit 2
fi

REF_BIN=$(realpath "$1")
NEW_BIN=$(realpath "$2")
SCENARIO=$(realpath "$3")
CEPAC_DIR=$(realpath "$4")
SIZE=${5:-}
DURATION=${6:-}
WORK_DIR=${WORK_DIR:-$(mktemp -d -t transm-regression.XXXXXX)}

# Files whose content depends on the wall clock or the machine, not the model.
VOLATILE='(\.cout|run\.log|time\.log|status\.db)$'
# CEPAC summaries: timestamped, and not reproducible run to run (see above).
CEPAC_SUMMARY='(run\.out|cepacPopstats\.out)$'

without_timestamps() {
    sed -E 's#[0-9]{2}/[0-9]{2}/[0-9]{2}[ ,\t]+[0-9]{2}:[0-9]{2}:[0-9]{2}#<time>#g' "$1"
}

SEED=${SEED:-}
seed=${SEED:-$(grep -o '<fixedSeed>-\?[0-9]*</fixedSeed>' "$SCENARIO" | grep -o -- '-\?[0-9]*' | head -1)}
if [[ -z "$seed" || "$seed" -lt 0 ]]; then
    echo "Scenario has no fixed seed; outputs would not be reproducible." >&2
    exit 2
fi

run_one() {
    local name=$1 bin=$2 dir="$WORK_DIR/$1"
    rm -rf "$dir" && mkdir -p "$dir"
    cp "$SCENARIO" "$dir/run.xml"
    # Only the population's initial <size> and the top-level <duration> are replaced.
    [[ -n "$SIZE" ]] && sed -i "0,/<size>[0-9]*<\/size>/s//<size>$SIZE<\/size>/" "$dir/run.xml"
    [[ -n "$SEED" ]] && sed -i "0,/<fixedSeed>-\?[0-9]*<\/fixedSeed>/s//<fixedSeed>$SEED<\/fixedSeed>/" "$dir/run.xml"
    [[ -n "$DURATION" ]] && sed -i "0,/<duration>[0-9]*<\/duration>/s//<duration>$DURATION<\/duration>/" "$dir/run.xml"
    ln -s "$CEPAC_DIR" "$dir/INFILES"
    local start=$SECONDS
    (cd "$dir" && "$bin" --cepac INFILES --focus off run.xml > run.log 2>&1)
    local status=$?
    echo "$name: exit $status in $((SECONDS - start)) s"
    return $status
}

crashed=0
run_one reference "$REF_BIN" || { echo "WARNING: reference run failed, see $WORK_DIR/reference/run.log"; crashed=1; }
run_one candidate "$NEW_BIN" || { echo "WARNING: candidate run failed, see $WORK_DIR/candidate/run.log"; crashed=1; }

ref_files=$(cd "$WORK_DIR/reference" && find . -type f ! -name run.xml | grep -Ev "$VOLATILE" | sort)
new_files=$(cd "$WORK_DIR/candidate" && find . -type f ! -name run.xml | grep -Ev "$VOLATILE" | sort)

failed=0
if [[ "$ref_files" != "$new_files" ]]; then
    echo "The two runs wrote different sets of files:"
    diff <(echo "$ref_files") <(echo "$new_files")
    failed=1
fi

compared=0
cepac_differs=0
while read -r f; do
    [[ -z "$f" || ! -f "$WORK_DIR/candidate/$f" ]] && continue
    compared=$((compared + 1))
    if [[ "$f" =~ $CEPAC_SUMMARY ]]; then
        if ! cmp -s <(without_timestamps "$WORK_DIR/reference/$f") <(without_timestamps "$WORK_DIR/candidate/$f"); then
            echo "WARNING: CEPAC summary differs: $f"
            cepac_differs=1
        fi
    elif ! cmp -s "$WORK_DIR/reference/$f" "$WORK_DIR/candidate/$f"; then
        echo "DIFFERS: $f"
        failed=1
    fi
done <<< "$ref_files"

if [[ $failed -ne 0 ]]; then
    echo "FAIL: outputs differ ($WORK_DIR)"
    exit 1
elif [[ $crashed -ne 0 || $cepac_differs -ne 0 ]]; then
    echo "PASS WITH WARNING: TRANSM outputs identical; see warnings above ($WORK_DIR)"
    exit 3
fi
echo "PASS: $compared output files identical ($WORK_DIR)"
exit 0

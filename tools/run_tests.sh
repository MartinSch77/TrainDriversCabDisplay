#!/usr/bin/env bash
# Run the whole test suite and record per-test-function results as JUnit XML
# in test-results/ — the result leg of the traceability chain
# (tools/trace_report.py joins them with the specs). ctest alone only records
# pass/fail per executable, so each test binary is run with its own JUnit
# writer (tests/rd_test.h, --junit flag).
#
# Usage: tools/run_tests.sh [build-dir]     (default: build)
set -euo pipefail

BUILD_DIR="${1:-build}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/test-results"
mkdir -p "$OUT"
rm -f "$OUT"/*.xml

FAIL=0
for exe in "$ROOT/$BUILD_DIR"/tests/tst_*; do
    [ -f "$exe" ] && [ -x "$exe" ] || continue
    case "$exe" in *.xml) continue ;; esac
    name="$(basename "$exe")"
    echo "=== $name ==="
    if ! "$exe" --junit "$OUT/$name.xml"; then
        FAIL=1
    fi
done

echo
echo "JUnit results in $OUT/"
exit $FAIL

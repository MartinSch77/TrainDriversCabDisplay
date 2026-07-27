#!/usr/bin/env bash
# Build the HTML documentation: refresh the requirements export and the
# traceability matrix (so the docs always ship the current trace state),
# then run doxygen.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

tools/make_requirements.sh              # StrictDoc export + regenerated requirements.md
python3 tools/trace_report.py || true   # gaps are reported inside the matrix
doxygen Doxyfile
echo "docs: $ROOT/docs/html/index.html"

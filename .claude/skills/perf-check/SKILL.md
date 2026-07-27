---
name: perf-check
description: Measure RailDeck Pro performance — the deterministic core tick benchmark (Debug vs release), optional CPU profiling of the frontends — and judge whether a change regressed the hot path. Use for "is it faster/slower", performance regressions, or before/after comparisons of optimisation work.
---

# Performance measurement (RailDeck Pro)

1. **Benchmark** (deterministic, no profiler needed):
   ```bash
   ./build_all.sh build release        # both trees
   ./build/tests/tst_benchmarks | grep RESULT
   ./build-release/tests/tst_benchmarks | grep RESULT
   ```
   Hot path covered: `TrainSimulation::tick` (TS-PERF-001, REQ-N-006 floor:
   > 20 000 ticks/s in Debug). Record a fresh Debug + release pair before and
   after the change; > 2× regression = investigate.
2. **Profiling** (hotspot drill-down): `tools/profile.sh [binary] [seconds]` —
   auto-detects perf or gperftools over the release Qt frontend (offscreen);
   if neither is installed it prints the two apt install lines (sudo needed,
   ask the user).
3. **Architecture invariants to preserve**: the UI tick is 30 Hz — one
   simulation tick must stay far below that budget (REQ-N-006); frontends
   render the `TrainState` snapshot without extra per-tick allocations
   (LVGL side repositions pre-created widgets, no churn); keep every boolean
   decision at ≤ 6 conditions (clang-18 MC/DC limit).
4. Report a before/after table with the benchmark numbers and a verdict.

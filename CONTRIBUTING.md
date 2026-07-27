# Contributing

Thanks for your interest! This project is a train cab display (HMI) with a
UI-agnostic C++17 simulation core, two interchangeable frontends (Qt Quick +
LVGL) and an unusually complete quality toolchain — contributions are
expected to keep every leg of it green.

## Getting started

```bash
./setup.sh            # provision all tools on a naked Debian/Ubuntu (idempotent)
./build_all.sh        # build both frontends + tests and generate every artefact
./clean_all.sh        # remove everything generated
```

Individual stages: `./build_all.sh build test` for the quick loop. The apps:
`./build/ui-qt/raildeck-qt` and `./build/ui-lvgl/raildeck-lvgl`.

## Quality bar for pull requests

1. **Tests pass**: `tools/run_tests.sh build` — zero failures. New behaviour
   needs a new test.
2. **Traceability intact**: `python3 tools/trace_report.py` must report no
   hard gaps. New tests carry the tag block (see `docs/test_spec.md`):
   `//! @tstid TS-… @design DES-…` + `// @relation(REQ-…, scope=function)`,
   and are registered with `RD_RUN(...)`. New requirements go into
   `requirements/requirements.sdoc` (StrictDoc, single source of truth) —
   `docs/requirements.md` is generated, never edit it by hand.
3. **Static analysis**: `tools/static_analysis.sh build` — do not add new
   cppcheck/clang-tidy findings (`.clang-tidy` documents the check set and
   the deliberate exclusions).
4. **Sanitizers stay clean**: `tools/sanitize.sh` (ASan+UBSan, TSan,
   valgrind) — CI runs the ASan leg on every PR.
5. **Style**: match the surrounding code; keep every boolean decision at
   ≤ 6 conditions (clang-18 MC/DC instrumentation limit).

## Architecture ground rules

- The core stays UI-free (REQ-N-001): frontends interact only through
  `tick()`, `setLever()`, `command()` and render the `TrainState` snapshot.
- Behaviour changes land in the core first, then in **both** frontends —
  Qt Quick and LVGL must stay functionally equivalent (REQ-N-002).
- Colors/fonts/metrics only via `design/theme.json` +
  `python3 design/generate_tokens.py` (REQ-N-003); never edit the generated
  `Theme.qml` / `theme_tokens.h`.
- The simulation stays deterministic (REQ-N-004): no wall clock, no
  randomness in `core/`.

## Commit messages

Short imperative subject, body explains the why. Reference requirement ids
(`REQ-…`) when the change affects specified behaviour.

# RailDeck Pro — project instructions

Train driver's cab display (train HMI): one UI-agnostic C++17 simulation core
(`traincore`) with THREE interchangeable frontends — Qt Quick (QML, Qt 6.5+),
LVGL v9.3 (SDL simulator) and Slint 1.17 (prebuilt C++ package, fetched by
CMake) — sharing one design-token source. Quality toolchain:
requirements-as-code, full traceability, four static analyzers +
three sanitizers on one Axivion dashboard.

## Entry points

```bash
./setup.sh                     # provision/update all tools (naked Debian/Ubuntu)
./build_all.sh                 # everything incl. Axivion; --skip axivion; app; release
./clean_all.sh [--deep]        # remove everything generated
tools/run_tests.sh build       # test suite with JUnit output
python3 tools/trace_report.py  # traceability matrix; fails on hard gaps
tools/static_analysis.sh build [--fix]   # cppcheck+clang-tidy+clazy+g++ -fanalyzer
tools/sanitize.sh [asan-ubsan|tsan|valgrind|all]
tools/coverage.sh              # gcov line/branch + clang-18 MC/DC (Coco if licensed)
```

Skills: `/verify` (all checks), `/axivion-dashboard` (run + REST verification),
`/add-requirement` (requirements-as-code workflow), `/perf-check` (benchmark).

## Non-negotiables

- The core stays UI-free: frontends interact ONLY via `tick()`, `setLever()`,
  `command()` and render the `TrainState` snapshot (REQ-N-001). New HMI
  behaviour goes into the core first, then into ALL THREE frontends.
- Colors/fonts/metrics come ONLY from `design/theme.json` →
  `python3 design/generate_tokens.py` regenerates `ui-qt/qml/Theme.qml`,
  `shared/theme_tokens.h` and `ui-slint/ui/Theme.slint`. Never edit the
  generated files (REQ-N-003).
- Requirements live ONLY in `requirements/requirements.sdoc` (StrictDoc);
  `docs/requirements.md` is generated (`tools/make_requirements.sh`).
- Every test carries `//! @tstid TS-… @design DES-…` plus
  `// @relation(REQ-…, scope=function)` (plain `//` — StrictDoc ignores
  `//!`), and is registered with `RD_RUN(...)` in the binary's `main()`.
- Keep every boolean decision ≤ 6 conditions (clang-18 MC/DC limit).
- The simulation must stay deterministic — no wall clock, no randomness in
  `traincore` (REQ-N-004); ripple effects use `sin` over sim time.
- ONE Axivion run at a time (flock in `axivion/start_analysis.sh`); no
  clean/build while it runs. It analyzes with `RAILDECK_UI=qt` so fetched
  LVGL sources stay out of MISRA. External findings import:
  `axivion/external_import.py` (Python layer — matchlist is not expressible
  in the JSON configs).
- Check `.clang-tidy` header comments before disabling checks; disable only
  with a written rationale.

## Gotchas that cost hours

- WSL2: Qt 6.11.x under `/mnt/c/Qt` is the WINDOWS kit (MSVC/MinGW) — build
  with the Linux kit `~/Qt/6.10.2/gcc_64` (`QT_PREFIX`).
- LVGL desktop builds need `LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB` in
  `ui-lvgl/lv_conf.h`: the default 64 KB builtin TLSF pool makes
  `lv_obj_create` spin forever once the full cab UI exhausts it.
- LVGL v9.3 CMake wants `LV_BUILD_CONF_PATH` (not `LV_CONF_PATH`).
- Verification screenshots: all three binaries take
  `--screenshot <file> [--screenshot-delay ms]`. Qt offscreen works, but
  Qt Quick 3D needs a real display (`DISPLAY=:0` under WSLg); LVGL and Slint
  write BMP. Slint under xvfb/CI: set `SLINT_BACKEND=winit-software`.
- Montserrat (LVGL built-in font) covers ASCII only — no ‰, ·, →, ▲; use
  ASCII ("mm/m", "-", LV_SYMBOL_*) in the LVGL frontend.
- Slint 1.17: `rotation-angle` is deprecated/rejected on most elements — use
  `transform-rotation` (rotates the element AND its children around the
  element centre, so QML's "rotate a full-size Item with an offset child"
  pattern ports 1:1). Fixed decimals ("%.1f") are not expressible in .slint;
  such strings come pre-formatted from `ui-slint/src/main.cpp`.

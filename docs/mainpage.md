# RailDeck Pro — Developer Documentation

@mainpage RailDeck Pro

Train driver's cab display (train HMI) with two interchangeable UI frontends —
Qt Quick (QML) and LVGL — sharing one UI-agnostic simulation core
(`traincore`) and one design-token source (`design/theme.json`).

## Documentation map

- @ref requirements — Software Requirement Specification (generated from
  `requirements/requirements.sdoc`, the single source of truth)
- @ref design — Software design: design elements (`DES-…`) and the
  requirements they satisfy
- @ref test_spec — Test specification (`TS-…` cases, 1:1 with the test
  functions in `tests/tst_*.cpp`)
- @ref traceability — the generated requirements ↔ design ↔ test ↔ result
  matrix

## Architecture in one paragraph

`core/` holds the deterministic train simulation (physics, supervision,
vigilance, doors, pneumatics, electrics, communication, route). A frontend
owns one `traincore::TrainSimulation`, calls `tick()` from its own timer,
forwards operator input through `setLever()`/`command()`, and renders the
`TrainState` snapshot — nothing else crosses the boundary. Both frontends are
styled exclusively from tokens generated out of `design/theme.json`
(`ui-qt/qml/Theme.qml`, `shared/theme_tokens.h`), which is what keeps the look
& feel identical.

## Quality pipeline

`./build_all.sh` drives everything: build, tests (JUnit per test function),
traceability gate, docs, coverage (line/branch + MC/DC), static analysis
(cppcheck, clang-tidy, clazy, g++ -fanalyzer, codespell), sanitizers
(ASan+UBSan, TSan, valgrind) and the Axivion analysis. See the repository
README and `CONTRIBUTING.md`.

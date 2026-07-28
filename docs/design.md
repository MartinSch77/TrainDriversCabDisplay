# RailDeck Pro — Software Design (constructive counterpart)

@page design Software Design
@tableofcontents

Design element IDs (`DES-…`) are the constructive counterpart in the
traceability chain: requirement → design element → test. Each element names
the implementing classes/files and the requirements it satisfies
(`satisfies:` list is machine-read by `tools/trace_report.py`).

## Simulation core (`core/`, target `traincore`)

| ID | Element | Implementation | satisfies |
|----|---------|----------------|-----------|
| DES-CORE-SIM | UI-agnostic train simulation: tick loop, longitudinal dynamics (traction curve, resistance, gradient), deterministic state evolution | `train_simulation.h/.cpp` (`TrainSimulation::tick`, `tickMotion`), `train_types.h` (`TrainState`) | REQ-N-001, REQ-N-004, REQ-N-006 |
| DES-CORE-SUP | Speed supervision: static limits, braking curves to lower limits and station stops, overspeed warning + latched intervention | `tickSupervision` | REQ-F-001, REQ-F-002, REQ-F-003 |
| DES-CORE-SIFA | Vigilance device: armed → visual → audible → penalty brake, release rule at standstill | `tickSifa`, `Command::SifaAcknowledge` | REQ-F-004, REQ-F-005 |
| DES-CORE-DOOR | Door state machines per side, standstill release rule, traction interlock | `tickDoors`, `stepDoor`, `Command::Doors*` | REQ-F-006, REQ-F-007 |
| DES-CORE-BRAKE | Blended braking: electrodynamic first with speed fade, pneumatic supplement; emergency brake latched until standstill | `tickPneumatics`, `tickMotion`, `Command::EmergencyBrakeToggle` | REQ-F-012, REQ-F-017 |
| DES-CORE-PNEU | Pneumatic plant: brake pipe, brake cylinder, main reservoir with compressor hysteresis | `tickPneumatics` | REQ-F-017 |
| DES-CORE-ELEC | Electric plant: pantograph, line voltage, motor current, power/regeneration, energy counter | `tickElectric`, `Command::PantographToggle` | REQ-F-010, REQ-F-017 |
| DES-CORE-COMMS | PA channel and control-centre radio call state machine | `tickComms`, `Command::PaToggle`, `Command::RadioToggle` | REQ-F-008, REQ-F-009 |
| DES-CORE-AFB | Cruise control: proportional lever demand towards min(set, permitted), step adjustment with clamping | `tickMotion` (AFB block), `Command::Afb*` | REQ-F-011 |
| DES-CORE-ADV | Driving advisor heuristics (power/hold/coast/brake) | `tickAdvisor` | REQ-F-013 |
| DES-CORE-ALERT | Alert collection ordered by severity | `rebuildAlerts` | REQ-F-014 |
| DES-CORE-ROUTE | Route model: segments, stations, gradient lookup, next-station resolution, clock/odometer | `tickJourney`, `limitAtM`, `gradientAtM`, `route()`, `stations()` | REQ-F-015, REQ-F-016 |

## Design tokens (`design/`)

| ID | Element | Implementation | satisfies |
|----|---------|----------------|-----------|
| DES-TOK-GEN | Single-source theme: `theme.json` generated into the QML singleton, the C header and the Slint global | `design/theme.json`, `design/generate_tokens.py` → `ui-qt/qml/Theme.qml`, `shared/theme_tokens.h`, `ui-slint/ui/Theme.slint` | REQ-N-003 |

## Qt Quick frontend (`ui-qt/`, target `raildeck-qt`)

| ID | Element | Implementation | satisfies |
|----|---------|----------------|-----------|
| DES-QT-FRONT | Thin QML frontend: backend adapter (30 Hz tick, one notify signal), speedometer with supervision ring, gauges, effort bar, planning strip, 3D line-ahead view, button bar, keyboard controls, `--screenshot` support | `src/trainbackend.h/.cpp`, `src/main.cpp`, `qml/*.qml` | REQ-N-001, REQ-N-002, REQ-N-005, REQ-F-001, REQ-F-008, REQ-F-009, REQ-F-013, REQ-F-014, REQ-F-015, REQ-F-016, REQ-F-017 |

## LVGL frontend (`ui-lvgl/`, target `raildeck-lvgl`)

| ID | Element | Implementation | satisfies |
|----|---------|----------------|-----------|
| DES-LVGL-FRONT | Thin LVGL frontend mirroring the Qt layout 1:1 from the shared tokens: arcs/scales/bars, perspective line-ahead view, planning strip, button bar, SDL simulator, BMP `--screenshot` support | `src/ui.h/.cpp`, `src/main.cpp`, `lv_conf.h` | REQ-N-001, REQ-N-002, REQ-N-005, REQ-F-001, REQ-F-008, REQ-F-009, REQ-F-013, REQ-F-014, REQ-F-015, REQ-F-016, REQ-F-017 |

## Slint frontend (`ui-slint/`, target `raildeck-slint`)

| ID | Element | Implementation | satisfies |
|----|---------|----------------|-----------|
| DES-SLINT-FRONT | Thin Slint frontend mirroring the Qt layout 1:1 from the shared tokens: declarative `.slint` components (speedometer with supervision ring, gauges, effort bar, planning strip, perspective line-ahead view, button bar, keyboard controls), C++ bridge pumping one snapshot per tick into the Backend global, BMP `--screenshot` support | `src/main.cpp`, `ui/*.slint` | REQ-N-001, REQ-N-002, REQ-N-005, REQ-F-001, REQ-F-008, REQ-F-009, REQ-F-013, REQ-F-014, REQ-F-015, REQ-F-016, REQ-F-017 |

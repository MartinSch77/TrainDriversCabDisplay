# RailDeck Pro — Test Specification

@page test_spec Test Specification
@tableofcontents

Requirement-based test specification. Every test case has a stable ID
(`TS-…`), links to the requirement(s) it verifies and the design element it
exercises, and maps 1:1 to a test function of the same name in
`tests/tst_*.cpp` (tagged there with `@tstid` / `@design` plus the StrictDoc
source marker `@relation(REQ-…, scope=function)` naming the verified
requirements).
Test results are read from the JUnit XML files the suite writes to
`test-results/` (see `tools/run_tests.sh`); the traceability matrix
(`tools/trace_report.py`) joins spec ↔ implementation ↔ result ↔ design ↔
requirement and reports gaps.

Levels: **U** = unit (single subsystem), **S** = scenario (closed-loop
simulation over time).

## Vigilance device (tests/tst_core.cpp, DES-CORE-SIFA, REQ-F-004/005)

| ID | L | Case |
|----|---|------|
| TS-SIFA-001 | S | Unacknowledged SIFA escalates to the penalty brake; the train is braked to a stand and the brake pipe is vented. |
| TS-SIFA-002 | S | Acknowledge while moving does NOT release the penalty brake; acknowledge at standstill does, and the brake pipe recharges. |

## Doors (tests/tst_core.cpp, DES-CORE-DOOR, REQ-F-006/007)

| ID | L | Case |
|----|---|------|
| TS-DOOR-001 | U | A door release command while moving is refused. |
| TS-DOOR-002 | S | At standstill: release → door opens; close → door closes and locks. |
| TS-DOOR-003 | S | An unsecured door inhibits tractive effort; locking restores it. |

## Speed supervision (tests/tst_core.cpp, DES-CORE-SUP, REQ-F-001/002/003)

| ID | L | Case |
|----|---|------|
| TS-SUP-001 | S | The permitted speed never exceeds the line limit; a braking-curve target (target speed + distance) appears ahead of a restriction. |
| TS-SUP-002 | S | Driving at full power raises the overspeed warning, then a brake intervention that only releases below the permitted speed. |

## Cruise control (tests/tst_core.cpp, DES-CORE-AFB, REQ-F-011)

| ID | L | Case |
|----|---|------|
| TS-AFB-001 | S | AFB holds the 140 km/h set speed within a tight band. |
| TS-AFB-002 | U | Set speed adjusts in steps and clamps at 0 and 200 km/h; AFB toggles on/off. |

## Communication (tests/tst_core.cpp, DES-CORE-COMMS, REQ-F-008/009/014)

| ID | L | Case |
|----|---|------|
| TS-COM-001 | U | PA toggles live/off and a live channel is reported as an alert. |
| TS-COM-002 | S | Radio: calling → connected after the connect delay; toggling ends the call. |

## Electric plant (tests/tst_core.cpp, DES-CORE-ELEC, REQ-F-010/017)

| ID | L | Case |
|----|---|------|
| TS-ELEC-001 | S | Lowering the pantograph collapses line voltage, tractive effort and drawn power. |
| TS-ELEC-002 | S | Under traction: line voltage in the nominal band, positive current, power and effort. |

## Pneumatics (tests/tst_core.cpp, DES-CORE-PNEU, REQ-F-017)

| ID | L | Case |
|----|---|------|
| TS-PNEU-001 | S | Released: brake pipe ~5.0 bar, reservoir in the compressor band. Full service braking drops the pipe and fills the cylinder (blended braking: pneumatic supplement beyond the electrodynamic share). |

## Braking (tests/tst_core.cpp, DES-CORE-BRAKE, REQ-F-012)

| ID | L | Case |
|----|---|------|
| TS-BRAKE-001 | S | The emergency brake applies, cannot be released while moving, stops the train, and releases at standstill. |

## Advisor (tests/tst_core.cpp, DES-CORE-ADV, REQ-F-013)

| ID | L | Case |
|----|---|------|
| TS-ADV-001 | S | POWER is suggested well below the permitted speed; BRAKE is suggested on overspeed. |

## Alerts (tests/tst_core.cpp, DES-CORE-ALERT, REQ-F-014)

| ID | L | Case |
|----|---|------|
| TS-ALERT-001 | S | The penalty brake raises a critical alert naming SIFA, listed first. |

## Journey & route (tests/tst_core.cpp, DES-CORE-ROUTE, REQ-F-015/016)

| ID | L | Case |
|----|---|------|
| TS-ROUTE-001 | U | Next station and distance resolve correctly; route profile, stations and current gradient are exposed. |

## Determinism (tests/tst_core.cpp, DES-CORE-SIM, REQ-N-004)

| ID | L | Case |
|----|---|------|
| TS-DET-001 | S | Two simulations fed the identical command/tick sequence end bit-identical in speed, position and energy. |

## Performance (tests/tst_benchmarks.cpp, DES-CORE-SIM, REQ-N-006)

| ID | L | Case |
|----|---|------|
| TS-PERF-001 | U | Sustained simulation tick rate above 20 000 ticks/s (prints the measured RESULT line for trend comparison). |

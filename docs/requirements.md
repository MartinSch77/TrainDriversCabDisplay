# RailDeck Pro — Software Requirement Specification (SRS)

@page requirements Software Requirements
@tableofcontents

<!-- GENERATED FILE — do not edit. Source of truth:
     requirements/requirements.sdoc (StrictDoc). Regenerate with
     tools/make_requirements.sh -->

This page is **generated** from `requirements/requirements.sdoc`
(requirements-as-code, StrictDoc) — edit there and run
`tools/make_requirements.sh`. Requirement IDs are stable and referenced
from the design (`docs/design.md`), the test specification
(`docs/test_spec.md`), the test implementations (`tests/tst_*.cpp`, marker
`@relation(REQ-…, scope=function)`) and the generated traceability matrix
(`docs/traceability.html`). Format: `REQ-F-xxx` functional, `REQ-N-xxx`
non-functional. Verification method: T = test, A = analysis,
I = inspection.

## Functional requirements

| ID | Requirement | Verify |
|----|-------------|--------|
| REQ-F-001 | The cab display shall show the current speed on a circular speedometer together with the currently permitted speed; the permitted speed shall never exceed the applicable line speed limit. | T, I |
| REQ-F-002 | The system shall supervise the speed against braking curves towards lower speed limits and station stops ahead, and shall present the governing target speed and the distance to that target. | T, I |
| REQ-F-003 | The system shall raise an overspeed warning when the speed exceeds the permitted speed, and shall apply an automatic brake intervention on gross overspeed that is only released once the speed is back below the permitted speed. | T |
| REQ-F-004 | The vigilance device shall require periodic driver acknowledgement while the train is moving and shall escalate through a visual warning and an audible warning to a penalty brake application when unacknowledged. | T |
| REQ-F-005 | A vigilance penalty brake application shall only be releasable after the train has come to a standstill and the driver has acknowledged. | T |
| REQ-F-006 | Door release commands (per side) shall only be accepted at standstill; released doors shall open for passenger exchange and shall close and lock on the close command. | T |
| REQ-F-007 | Tractive effort shall be inhibited while any door is not closed and locked. | T |
| REQ-F-008 | The driver shall be able to open and close a public-address channel to the passengers; a live channel shall be clearly indicated. | T, I |
| REQ-F-009 | The driver shall be able to establish and end a voice-radio call to the traffic control centre; the call state (calling, connected) shall be indicated. | T, I |
| REQ-F-010 | The driver shall be able to raise and lower the pantograph; with the pantograph lowered there shall be no line voltage, no drawn power and no tractive effort. | T |
| REQ-F-011 | A cruise control shall hold an adjustable set speed, limited by the permitted speed; the set speed shall be adjustable in steps and clamped to the valid range. | T |
| REQ-F-012 | An emergency brake command shall apply the full emergency brake force until standstill and shall only be releasable at standstill. | T |
| REQ-F-013 | A driving advisor shall recommend the energy-efficient action (power, hold, coast, brake) for the current driving situation. | T, I |
| REQ-F-014 | Active abnormal conditions (vigilance warnings, overspeed, unsecured doors, lowered pantograph, live communication channels, low air pressure) shall be reported as alerts ordered by severity, most severe first. | T, I |
| REQ-F-015 | The display shall show the next station with its remaining distance, the service identifier and the current time. | T, I |
| REQ-F-016 | The route profile (speed-limit changes, station positions, gradients) shall be available to the frontends for the planning strip and the line-ahead preview. | T, I |
| REQ-F-017 | The display shall present the pneumatic values (brake pipe, brake cylinder, main reservoir pressure) and the electric values (line voltage, motor current, tractive effort, power, energy) with physically plausible behaviour. | T, I |

## Non-functional requirements

| ID | Requirement | Verify |
|----|-------------|--------|
| REQ-N-001 | All HMI behaviour shall live in a UI-independent C++ core library; a frontend shall interact with it exclusively through tick(), setLever() and command() and shall render the published state snapshot. | A, I |
| REQ-N-002 | The Qt Quick and the LVGL frontend shall provide the same instrument layout, functions and colors, driven by the same core API. | I |
| REQ-N-003 | All colors, font sizes and key metrics shall be generated from the single source design/theme.json for every frontend; generated token files shall not be edited by hand. | A, I |
| REQ-N-004 | Given the same command and tick sequence, the simulation core shall reproduce the identical state evolution (no wall-clock or randomness dependencies). | T |
| REQ-N-005 | Both frontends shall support automated headless screenshot capture (--screenshot, --screenshot-delay) for visual verification. | I |
| REQ-N-006 | >>> | T |

// Requirement-based tests for the UI-agnostic simulation core. Every test
// function carries its traceability tag block (see docs/test_spec.md):
// `//! @tstid TS-… @design DES-…` names the spec row and design element,
// `// @relation(REQ-…, scope=function)` names the verified requirement(s).
#include "rd_test.h"
#include "traincore/train_simulation.h"

#include <cmath>

using namespace traincore;

namespace {

void run(TrainSimulation &sim, double seconds)
{
    for (int i = 0; i < static_cast<int>(seconds / 0.05); ++i)
        sim.tick(0.05);
}

// Full service brake to standstill, acknowledging the vigilance device.
void stopTrain(TrainSimulation &sim)
{
    sim.setLever(-100);
    run(sim, 60.0);
    sim.command(Command::SifaAcknowledge);
    sim.setLever(0);
}

} // namespace

// ---------------------------------------------------------------- vigilance

//! @tstid TS-SIFA-001 @design DES-CORE-SIFA
// @relation(REQ-F-004, scope=function)
static void TS_SIFA_001_escalatesToPenaltyBrake()
{
    TrainSimulation sim;
    run(sim, 30.0);
    RD_CHECK(sim.state().sifa == SifaStage::EmergencyBrake,
             "SIFA reaches penalty brake without acknowledgement");
    run(sim, 120.0);
    RD_CHECK(sim.state().speedKmh < 0.1, "penalty brake stops the train");
    RD_CHECK(sim.state().brakePipeBar < 0.2, "emergency vents the brake pipe");
}

//! @tstid TS-SIFA-002 @design DES-CORE-SIFA
// @relation(REQ-F-005, scope=function)
static void TS_SIFA_002_penaltyReleasesOnlyAtStandstill()
{
    TrainSimulation sim;
    run(sim, 25.0); // penalty latched, still moving
    sim.command(Command::SifaAcknowledge);
    run(sim, 1.0);
    RD_CHECK(sim.state().sifa == SifaStage::EmergencyBrake,
             "acknowledge while moving does not release the penalty brake");
    run(sim, 120.0); // brakes to a stand
    sim.command(Command::SifaAcknowledge);
    sim.setLever(0);
    run(sim, 1.0);
    RD_CHECK(sim.state().sifa != SifaStage::EmergencyBrake,
             "acknowledge at standstill releases the penalty brake");
    run(sim, 15.0);
    RD_CHECK(sim.state().brakePipeBar > 4.0, "brake pipe recharges after release");
}

// -------------------------------------------------------------------- doors

//! @tstid TS-DOOR-001 @design DES-CORE-DOOR
// @relation(REQ-F-006, scope=function)
static void TS_DOOR_001_releaseRefusedWhileMoving()
{
    TrainSimulation sim; // boots at 126 km/h
    sim.command(Command::DoorsLeftRelease);
    RD_CHECK(sim.state().doorLeft == DoorState::Locked,
             "door release refused while moving");
}

//! @tstid TS-DOOR-002 @design DES-CORE-DOOR
// @relation(REQ-F-006, scope=function)
static void TS_DOOR_002_releaseOpenCloseCycleAtStandstill()
{
    TrainSimulation sim;
    stopTrain(sim);
    sim.command(Command::DoorsLeftRelease);
    run(sim, 5.0);
    RD_CHECK(sim.state().doorLeft == DoorState::Open, "released door opens");
    sim.command(Command::DoorsLeftClose);
    run(sim, 4.0);
    RD_CHECK(sim.state().doorLeft == DoorState::Locked, "door closes and locks");
}

//! @tstid TS-DOOR-003 @design DES-CORE-DOOR
// @relation(REQ-F-007, scope=function)
static void TS_DOOR_003_tractionInterlock()
{
    TrainSimulation sim;
    stopTrain(sim);
    sim.command(Command::DoorsLeftRelease);
    run(sim, 5.0);
    RD_CHECK(sim.state().tractionLocked, "open door locks traction");
    sim.setLever(100);
    run(sim, 3.0);
    RD_CHECK(sim.state().tractionEffortKn < 1.0, "interlock cuts tractive effort");
    sim.command(Command::DoorsLeftClose);
    run(sim, 4.0);
    RD_CHECK(!sim.state().tractionLocked, "interlock releases when doors lock");
    run(sim, 5.0);
    RD_CHECK(sim.state().tractionEffortKn > 50.0, "traction available again");
}

// -------------------------------------------------------------- supervision

//! @tstid TS-SUP-001 @design DES-CORE-SUP
// @relation(REQ-F-001, scope=function)
// @relation(REQ-F-002, scope=function)
static void TS_SUP_001_brakingCurveTargetAppears()
{
    TrainSimulation sim;
    bool sawCurve = false;
    bool permittedWithinLimit = true;
    sim.command(Command::AfbToggle);
    for (int i = 0; i < 40 && !sawCurve; ++i) {
        run(sim, 10.0);
        sim.command(Command::SifaAcknowledge);
        const TrainState &s = sim.state();
        if (s.permittedSpeedKmh > 200.0 + 0.001)
            permittedWithinLimit = false;
        if (s.distanceToTargetM >= 0 && s.targetSpeedKmh < s.permittedSpeedKmh)
            sawCurve = true;
    }
    RD_CHECK(permittedWithinLimit, "permitted speed never exceeds the line limit");
    RD_CHECK(sawCurve, "braking-curve target appears ahead of a restriction");
}

//! @tstid TS-SUP-002 @design DES-CORE-SUP
// @relation(REQ-F-003, scope=function)
static void TS_SUP_002_overspeedWarningAndIntervention()
{
    TrainSimulation sim;
    sim.setLever(100); // full power against the 160 km/h limit
    bool sawWarning = false;
    bool sawIntervention = false;
    for (int i = 0; i < 60; ++i) {
        run(sim, 2.0);
        if (i % 5 == 0)
            sim.command(Command::SifaAcknowledge);
        const TrainState &s = sim.state();
        if (s.overspeedWarning)
            sawWarning = true;
        if (s.brakeIntervention)
            sawIntervention = true;
        if (sawIntervention && !s.brakeIntervention) {
            RD_CHECK(s.speedKmh <= s.permittedSpeedKmh + 1.0,
                     "intervention releases only below the permitted speed");
            break;
        }
    }
    RD_CHECK(sawWarning, "overspeed warning raised above the permitted speed");
    RD_CHECK(sawIntervention, "brake intervention triggered on gross overspeed");
}

// ------------------------------------------------------------------- cruise

//! @tstid TS-AFB-001 @design DES-CORE-AFB
// @relation(REQ-F-011, scope=function)
static void TS_AFB_001_holdsSetSpeed()
{
    TrainSimulation sim;
    sim.command(Command::AfbToggle); // set speed boots at 140 km/h
    for (int i = 0; i < 9; ++i) {
        run(sim, 10.0);
        sim.command(Command::SifaAcknowledge);
    }
    const double v = sim.state().speedKmh;
    RD_CHECK(v > 135.0 && v < 143.0, "AFB holds the 140 km/h set speed");
}

//! @tstid TS-AFB-002 @design DES-CORE-AFB
// @relation(REQ-F-011, scope=function)
static void TS_AFB_002_setSpeedAdjustableAndClamped()
{
    TrainSimulation sim;
    sim.command(Command::AfbToggle);
    RD_CHECK(sim.state().afbEnabled, "AFB engages");
    for (int i = 0; i < 10; ++i)
        sim.command(Command::AfbIncrease);
    RD_CHECK(sim.state().afbSetKmh <= 200.0, "set speed clamps at 200 km/h");
    for (int i = 0; i < 30; ++i)
        sim.command(Command::AfbDecrease);
    RD_CHECK(sim.state().afbSetKmh >= 0.0, "set speed clamps at 0 km/h");
    sim.command(Command::AfbToggle);
    RD_CHECK(!sim.state().afbEnabled, "AFB disengages");
}

// ----------------------------------------------------------- communication

//! @tstid TS-COM-001 @design DES-CORE-COMMS
// @relation(REQ-F-008, scope=function)
// @relation(REQ-F-014, scope=function)
static void TS_COM_001_paToggleAndAlert()
{
    TrainSimulation sim;
    sim.command(Command::PaToggle);
    sim.tick(0.05);
    RD_CHECK(sim.state().paActive, "PA goes live");
    bool alerted = false;
    for (const Alert &a : sim.state().alerts)
        if (a.text.find("PA") != std::string::npos)
            alerted = true;
    RD_CHECK(alerted, "live PA is reported as an alert");
    sim.command(Command::PaToggle);
    sim.tick(0.05);
    RD_CHECK(!sim.state().paActive, "PA switches off");
}

//! @tstid TS-COM-002 @design DES-CORE-COMMS
// @relation(REQ-F-009, scope=function)
static void TS_COM_002_radioCallConnects()
{
    TrainSimulation sim;
    sim.command(Command::RadioToggle);
    RD_CHECK(sim.state().radio == RadioState::Calling, "radio starts calling");
    run(sim, 3.0);
    RD_CHECK(sim.state().radio == RadioState::Connected,
             "call connects to the control centre");
    sim.command(Command::RadioToggle);
    RD_CHECK(sim.state().radio == RadioState::Idle, "call ends");
}

// ----------------------------------------------------------------- electric

//! @tstid TS-ELEC-001 @design DES-CORE-ELEC
// @relation(REQ-F-010, scope=function)
static void TS_ELEC_001_pantographDownKillsPower()
{
    TrainSimulation sim; // boots with pantograph up, lever +30
    sim.command(Command::PantographToggle);
    run(sim, 3.0);
    const TrainState &s = sim.state();
    RD_CHECK(!s.pantographUp, "pantograph lowers");
    RD_CHECK(s.lineVoltageKv < 1.0, "line voltage collapses");
    RD_CHECK(s.tractionEffortKn < 0.5, "no tractive effort without line voltage");
    RD_CHECK(std::abs(s.powerMw) < 0.01, "no power drawn without line voltage");
}

//! @tstid TS-ELEC-002 @design DES-CORE-ELEC
// @relation(REQ-F-017, scope=function)
static void TS_ELEC_002_electricValuesPlausibleUnderPower()
{
    TrainSimulation sim;
    run(sim, 5.0);
    const TrainState &s = sim.state();
    RD_CHECK(s.lineVoltageKv > 13.5 && s.lineVoltageKv < 16.5,
             "line voltage inside the nominal 15 kV band");
    RD_CHECK(s.motorCurrentA > 0.0, "motor current flows under traction");
    RD_CHECK(s.powerMw > 0.0, "power is drawn under traction");
    RD_CHECK(s.tractionEffortKn > 0.0, "positive tractive effort with lever +30");
}

// --------------------------------------------------------------- pneumatics

//! @tstid TS-PNEU-001 @design DES-CORE-PNEU
// @relation(REQ-F-017, scope=function)
static void TS_PNEU_001_brakePneumatics()
{
    TrainSimulation sim;
    run(sim, 2.0);
    RD_CHECK(sim.state().brakePipeBar > 4.8 && sim.state().brakePipeBar < 5.2,
             "brake pipe at ~5.0 bar when released");
    RD_CHECK(sim.state().mainReservoirBar > 6.5 && sim.state().mainReservoirBar < 10.5,
             "main reservoir inside the compressor band");
    // Full service braking: the electrodynamic brake covers moderate demand
    // alone at speed (blended braking), so only full demand needs the
    // pneumatic supplement that shows on the cylinder gauge.
    sim.setLever(-100);
    run(sim, 5.0);
    RD_CHECK(sim.state().brakePipeBar < 4.0, "service braking drops the brake pipe");
    RD_CHECK(sim.state().brakeCylinderBar > 0.3,
             "brake cylinder fills on full service braking");
}

// ------------------------------------------------------------------ braking

//! @tstid TS-BRAKE-001 @design DES-CORE-BRAKE
// @relation(REQ-F-012, scope=function)
static void TS_BRAKE_001_emergencyBrakeStopsAndLatches()
{
    TrainSimulation sim;
    sim.command(Command::EmergencyBrakeToggle);
    RD_CHECK(sim.state().emergencyBrake, "emergency brake applies");
    run(sim, 5.0);
    sim.command(Command::EmergencyBrakeToggle); // release attempt while moving
    RD_CHECK(sim.state().emergencyBrake,
             "emergency brake cannot be released while moving");
    run(sim, 120.0);
    RD_CHECK(sim.state().speedKmh < 0.1, "emergency brake stops the train");
    sim.command(Command::EmergencyBrakeToggle);
    RD_CHECK(!sim.state().emergencyBrake, "release possible at standstill");
}

// ------------------------------------------------------------------ advisor

//! @tstid TS-ADV-001 @design DES-CORE-ADV
// @relation(REQ-F-013, scope=function)
static void TS_ADV_001_advisorHints()
{
    TrainSimulation sim;
    run(sim, 1.0); // 126 km/h, permitted 160 -> well below
    RD_CHECK(sim.state().advisor == AdvisorHint::Power,
             "advisor suggests POWER well below the permitted speed");
    sim.setLever(100);
    bool sawBrakeHint = false;
    for (int i = 0; i < 30 && !sawBrakeHint; ++i) {
        run(sim, 2.0);
        sim.command(Command::SifaAcknowledge);
        if (sim.state().overspeedWarning && sim.state().advisor == AdvisorHint::Brake)
            sawBrakeHint = true;
    }
    RD_CHECK(sawBrakeHint, "advisor suggests BRAKE on overspeed");
}

// ------------------------------------------------------------------- alerts

//! @tstid TS-ALERT-001 @design DES-CORE-ALERT
// @relation(REQ-F-014, scope=function)
static void TS_ALERT_001_criticalAlertFirst()
{
    TrainSimulation sim;
    run(sim, 30.0); // unacknowledged SIFA -> penalty brake
    const TrainState &s = sim.state();
    RD_CHECK(!s.alerts.empty(), "alerts raised for the penalty brake");
    RD_CHECK(!s.alerts.empty() && s.alerts[0].severity == AlertSeverity::Critical,
             "most severe alert listed first");
    RD_CHECK(!s.alerts.empty() && s.alerts[0].text.find("SIFA") != std::string::npos,
             "penalty-brake alert names SIFA");
}

// ------------------------------------------------------------------ journey

//! @tstid TS-ROUTE-001 @design DES-CORE-ROUTE
// @relation(REQ-F-015, scope=function)
// @relation(REQ-F-016, scope=function)
static void TS_ROUTE_001_journeyAndRouteData()
{
    TrainSimulation sim;
    const TrainState &s = sim.state();
    RD_CHECK(s.nextStationName == "Lindau West", "next station resolved ahead");
    RD_CHECK(s.nextStationDistanceM > 16000 && s.nextStationDistanceM < 18000,
             "next-station distance plausible");
    RD_CHECK(sim.route().size() >= 8, "route profile exposed for the planning display");
    RD_CHECK(sim.stations().size() >= 3, "stations exposed for the planning display");
    RD_CHECK(std::abs(s.gradientPermille - (-4.0)) < 0.001,
             "gradient matches the current route segment");
}

// -------------------------------------------------------------- determinism

//! @tstid TS-DET-001 @design DES-CORE-SIM
// @relation(REQ-N-004, scope=function)
static void TS_DET_001_deterministicEvolution()
{
    TrainSimulation a;
    TrainSimulation b;
    for (TrainSimulation *sim : {&a, &b}) {
        sim->setLever(70);
        for (int i = 0; i < 1000; ++i) {
            if (i == 500)
                sim->command(Command::SifaAcknowledge);
            sim->tick(0.05);
        }
    }
    RD_CHECK(a.state().speedKmh == b.state().speedKmh,
             "identical inputs give identical speed");
    RD_CHECK(a.state().routePositionM == b.state().routePositionM,
             "identical inputs give identical position");
    RD_CHECK(a.state().energyKwh == b.state().energyKwh,
             "identical inputs give identical energy");
}

int main(int argc, char **argv)
{
    RD_RUN(TS_SIFA_001_escalatesToPenaltyBrake);
    RD_RUN(TS_SIFA_002_penaltyReleasesOnlyAtStandstill);
    RD_RUN(TS_DOOR_001_releaseRefusedWhileMoving);
    RD_RUN(TS_DOOR_002_releaseOpenCloseCycleAtStandstill);
    RD_RUN(TS_DOOR_003_tractionInterlock);
    RD_RUN(TS_SUP_001_brakingCurveTargetAppears);
    RD_RUN(TS_SUP_002_overspeedWarningAndIntervention);
    RD_RUN(TS_AFB_001_holdsSetSpeed);
    RD_RUN(TS_AFB_002_setSpeedAdjustableAndClamped);
    RD_RUN(TS_COM_001_paToggleAndAlert);
    RD_RUN(TS_COM_002_radioCallConnects);
    RD_RUN(TS_ELEC_001_pantographDownKillsPower);
    RD_RUN(TS_ELEC_002_electricValuesPlausibleUnderPower);
    RD_RUN(TS_PNEU_001_brakePneumatics);
    RD_RUN(TS_BRAKE_001_emergencyBrakeStopsAndLatches);
    RD_RUN(TS_ADV_001_advisorHints);
    RD_RUN(TS_ALERT_001_criticalAlertFirst);
    RD_RUN(TS_ROUTE_001_journeyAndRouteData);
    RD_RUN(TS_DET_001_deterministicEvolution);
    return rdtest::finish("tst_core", argc, argv);
}

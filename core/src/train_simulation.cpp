#include "traincore/train_simulation.h"

#include <algorithm>
#include <cmath>

namespace traincore {

namespace {
constexpr double kMassKg = 180000.0;         // 4-car EMU
constexpr double kMaxTractionKn = 240.0;
constexpr double kMaxPowerKw = 4200.0;
constexpr double kMaxDynBrakeKn = 150.0;     // electrodynamic (regenerative)
constexpr double kServiceBrakeKn = 190.0;    // full service, blended
constexpr double kEmergencyBrakeKn = 215.0;
constexpr double kCylinderKnPerBar = 55.0;
constexpr double kSupervisionDecel = 0.70;   // braking-curve supervision m/s^2
constexpr double kLookaheadM = 4000.0;

constexpr double kSifaCycleS = 15.0;         // armed -> visual warning
constexpr double kSifaVisualS = 4.0;         // visual -> audible
constexpr double kSifaAudibleS = 4.0;        // audible -> penalty brake

double kmh2ms(double v) { return v / 3.6; }
double ms2kmh(double v) { return v * 3.6; }

double approach(double current, double target, double ratePerS, double dt)
{
    const double step = ratePerS * dt;
    if (current < target) return std::min(current + step, target);
    return std::max(current - step, target);
}
} // namespace

TrainSimulation::TrainSimulation()
    // 40 km loop: mixed main line with two speed-restricted urban areas.
    : m_route{
          {0.0,     120.0,  2.0},
          {5000.0,   80.0,  0.0},
          {8000.0,  160.0, -4.0},
          {14000.0, 200.0,  0.0},
          {22000.0, 130.0,  6.0},
          {26500.0, 100.0, 12.0},
          {31000.0, 160.0, -8.0},
          {38000.0,  60.0,  0.0},
      }
    , m_stations{
          {5000.0,  "Falkenberg Hbf"},
          {26500.0, "Lindau West"},
          {38700.0, "Steinbach"},
      }
    , m_routeLengthM(40000.0)
    , m_sifaTimerS(3.0)
{
    // Boot into a live run so the display shows realistic values immediately.
    m_s.routePositionM = 9000.0;
    m_s.speedKmh = 126.0;
    m_s.leverPercent = 30.0;
    m_s.afbSetKmh = 140.0;
    tickJourney();
    tickSupervision(0.0);
    rebuildAlerts();
}

double TrainSimulation::limitAtM(double positionM) const
{
    double p = std::fmod(positionM, m_routeLengthM);
    if (p < 0) p += m_routeLengthM;
    double limit = m_route.back().limitKmh;
    for (const auto &seg : m_route)
        if (p >= seg.startM) limit = seg.limitKmh;
    return limit;
}

double TrainSimulation::gradientAtM(double positionM) const
{
    double p = std::fmod(positionM, m_routeLengthM);
    if (p < 0) p += m_routeLengthM;
    double g = m_route.back().gradientPermille;
    for (const auto &seg : m_route)
        if (p >= seg.startM) g = seg.gradientPermille;
    return g;
}

void TrainSimulation::setLever(double percent)
{
    if (m_s.afbEnabled) return; // cruise control owns the lever
    m_s.leverPercent = std::clamp(percent, -100.0, 100.0);
}

void TrainSimulation::command(Command cmd)
{
    TrainState &s = m_s;
    const bool standstill = s.speedKmh < 0.5;
    switch (cmd) {
    case Command::SifaAcknowledge:
        if (s.sifa == SifaStage::EmergencyBrake) {
            if (standstill) { // penalty brake releases only after a full stop
                m_sifaPenaltyLatched = false;
                m_sifaTimerS = 0.0;
            }
        } else {
            m_sifaTimerS = 0.0; // pedal press restarts the cycle at any stage
        }
        break;
    case Command::PaToggle:
        s.paActive = !s.paActive;
        break;
    case Command::RadioToggle:
        if (s.radio == RadioState::Idle) {
            s.radio = RadioState::Calling;
            m_radioTimerS = 0.0;
        } else {
            s.radio = RadioState::Idle;
        }
        break;
    case Command::DoorsLeftRelease:
        if (standstill && s.doorLeft == DoorState::Locked) {
            s.doorLeft = DoorState::Released;
            m_doorLeftTimerS = 0.0;
        }
        break;
    case Command::DoorsLeftClose:
        if (s.doorLeft != DoorState::Locked && s.doorLeft != DoorState::Closing) {
            s.doorLeft = DoorState::Closing;
            m_doorLeftTimerS = 0.0;
        }
        break;
    case Command::DoorsRightRelease:
        if (standstill && s.doorRight == DoorState::Locked) {
            s.doorRight = DoorState::Released;
            m_doorRightTimerS = 0.0;
        }
        break;
    case Command::DoorsRightClose:
        if (s.doorRight != DoorState::Locked && s.doorRight != DoorState::Closing) {
            s.doorRight = DoorState::Closing;
            m_doorRightTimerS = 0.0;
        }
        break;
    case Command::PantographToggle:
        s.pantographUp = !s.pantographUp;
        break;
    case Command::AfbToggle:
        s.afbEnabled = !s.afbEnabled;
        if (s.afbEnabled && s.afbSetKmh < 10.0)
            s.afbSetKmh = std::max(40.0, std::round(s.speedKmh / 10.0) * 10.0);
        break;
    case Command::AfbIncrease:
        s.afbSetKmh = std::min(200.0, s.afbSetKmh + 10.0);
        break;
    case Command::AfbDecrease:
        s.afbSetKmh = std::max(0.0, s.afbSetKmh - 10.0);
        break;
    case Command::EmergencyBrakeToggle:
        if (s.emergencyBrake) {
            if (standstill) s.emergencyBrake = false;
        } else {
            s.emergencyBrake = true;
        }
        break;
    }
}

void TrainSimulation::tick(double dtS)
{
    const double dt = std::clamp(dtS, 0.0, 0.1);
    m_s.simTimeS += dt;

    tickJourney();
    tickSupervision(dt);
    tickSifa(dt);
    tickDoors(dt);
    tickPneumatics(dt);
    tickMotion(dt);
    tickElectric(dt);
    tickComms(dt);
    tickAdvisor();
    rebuildAlerts();

    // Wall clock, 09:41:00 at power-up.
    const int total = static_cast<int>(m_s.simTimeS) + 9 * 3600 + 41 * 60;
    m_s.clockHour = (total / 3600) % 24;
    m_s.clockMinute = (total / 60) % 60;
    m_s.clockSecond = total % 60;
}

void TrainSimulation::tickJourney()
{
    TrainState &s = m_s;
    const double p = std::fmod(s.routePositionM, m_routeLengthM);
    s.gradientPermille = gradientAtM(p);

    // Nearest station ahead (with a 50 m grace window while at the platform).
    double bestD = m_routeLengthM;
    for (const auto &st : m_stations) {
        double d = std::fmod(st.positionM - p + 50.0 + m_routeLengthM, m_routeLengthM) - 50.0;
        if (d < bestD) {
            bestD = d;
            s.nextStationName = st.name;
            s.nextStationDistanceM = std::max(0.0, d);
        }
    }
}

void TrainSimulation::tickSupervision(double dt)
{
    (void)dt;
    TrainState &s = m_s;
    const double p = std::fmod(s.routePositionM, m_routeLengthM);
    const double vMs = kmh2ms(s.speedKmh);
    const double staticLimit = limitAtM(p);

    // Most restrictive braking curve towards any lower limit or station stop
    // within the look-ahead window.
    double permitted = staticLimit;
    double govTarget = staticLimit;
    double govDistance = -1.0;

    auto considerTarget = [&](double atM, double targetKmh) {
        double d = atM - p;
        if (d < 0) d += m_routeLengthM;
        if (d > kLookaheadM) return;
        const double vt = kmh2ms(targetKmh);
        const double vCurve =
            ms2kmh(std::sqrt(vt * vt + 2.0 * kSupervisionDecel * d));
        if (vCurve < permitted) {
            permitted = vCurve;
            govTarget = targetKmh;
            govDistance = d;
        }
    };

    for (const auto &seg : m_route)
        if (seg.limitKmh < staticLimit)
            considerTarget(seg.startM, seg.limitKmh);
    // Only supervise the nearest stop ahead (50 m platform grace window).
    const RouteStation *nextStop = nullptr;
    double nextStopD = m_routeLengthM;
    for (const auto &st : m_stations) {
        const double d =
            std::fmod(st.positionM - p + 50.0 + m_routeLengthM, m_routeLengthM) - 50.0;
        if (d >= 0.0 && d < nextStopD) {
            nextStopD = d;
            nextStop = &st;
        }
    }
    if (nextStop != nullptr)
        considerTarget(nextStop->positionM, 0.0);

    s.permittedSpeedKmh = permitted;
    s.targetSpeedKmh = govDistance >= 0 ? govTarget : permitted;
    s.distanceToTargetM = govDistance;

    s.overspeedWarning = s.speedKmh > permitted + 2.0;
    if (s.speedKmh > permitted + 7.0)
        m_interventionUntilBelowKmh = permitted - 2.0;
    if (m_interventionUntilBelowKmh >= 0 && s.speedKmh <= m_interventionUntilBelowKmh)
        m_interventionUntilBelowKmh = -1.0;
    s.brakeIntervention = m_interventionUntilBelowKmh >= 0;
    (void)vMs;
}

void TrainSimulation::tickSifa(double dt)
{
    TrainState &s = m_s;
    if (m_sifaPenaltyLatched) {
        s.sifa = SifaStage::EmergencyBrake;
        s.sifaCountdownS = 0.0;
        return;
    }
    if (s.speedKmh < 1.0) {
        s.sifa = SifaStage::Inactive;
        m_sifaTimerS = 0.0;
        s.sifaCountdownS = kSifaCycleS;
        return;
    }
    m_sifaTimerS += dt;
    if (m_sifaTimerS < kSifaCycleS) {
        s.sifa = SifaStage::Armed;
        s.sifaCountdownS = kSifaCycleS - m_sifaTimerS;
    } else if (m_sifaTimerS < kSifaCycleS + kSifaVisualS) {
        s.sifa = SifaStage::VisualWarning;
        s.sifaCountdownS = kSifaCycleS + kSifaVisualS - m_sifaTimerS;
    } else if (m_sifaTimerS < kSifaCycleS + kSifaVisualS + kSifaAudibleS) {
        s.sifa = SifaStage::AudibleWarning;
        s.sifaCountdownS = kSifaCycleS + kSifaVisualS + kSifaAudibleS - m_sifaTimerS;
    } else {
        m_sifaPenaltyLatched = true;
        s.sifa = SifaStage::EmergencyBrake;
        s.sifaCountdownS = 0.0;
    }
}

void TrainSimulation::stepDoor(DoorState &door, double &timer, double dt)
{
    timer += dt;
    switch (door) {
    case DoorState::Released:
        if (timer > 1.0) { door = DoorState::Opening; timer = 0.0; }
        break;
    case DoorState::Opening:
        if (timer > 2.0) { door = DoorState::Open; timer = 0.0; }
        break;
    case DoorState::Closing:
        if (timer > 2.5) { door = DoorState::Locked; timer = 0.0; }
        break;
    case DoorState::Locked:
    case DoorState::Open:
        break;
    }
}

void TrainSimulation::tickDoors(double dt)
{
    stepDoor(m_s.doorLeft, m_doorLeftTimerS, dt);
    stepDoor(m_s.doorRight, m_doorRightTimerS, dt);
    m_s.tractionLocked =
        m_s.doorLeft != DoorState::Locked || m_s.doorRight != DoorState::Locked;
}

void TrainSimulation::tickPneumatics(double dt)
{
    TrainState &s = m_s;

    double demand = std::max(0.0, -s.leverPercent / 100.0);
    if (s.brakeIntervention) demand = std::max(demand, 0.7);
    const bool emergency = s.emergencyBrake || s.sifa == SifaStage::EmergencyBrake;
    if (emergency) demand = 1.0;

    // Blended braking: electrodynamic first (fades out below ~25 km/h),
    // pneumatic supplies the remainder -> visible on the cylinder gauge.
    const double wanted = (emergency ? kEmergencyBrakeKn : kServiceBrakeKn) * demand;
    const double dynFade = std::clamp(s.speedKmh / 25.0, 0.0, 1.0);
    const double dynKn = emergency ? 0.0 : std::min(wanted, kMaxDynBrakeKn * dynFade);
    const double pneuKn = wanted - dynKn;

    const double pipeTarget = emergency ? 0.0 : 5.0 - 1.5 * demand;
    const double pipeRate = s.brakePipeBar > pipeTarget ? 2.0 : 0.35;
    s.brakePipeBar = approach(s.brakePipeBar, pipeTarget, pipeRate, dt);

    const double cylTarget = std::clamp(pneuKn / kCylinderKnPerBar, 0.0, 4.2);
    const double cylRate = s.brakeCylinderBar < cylTarget ? 2.5 : 1.5;
    s.brakeCylinderBar = approach(s.brakeCylinderBar, cylTarget, cylRate, dt);

    // Main reservoir with compressor hysteresis.
    double consumption = 0.004;
    if (s.brakePipeBar < pipeTarget - 0.05) consumption += 0.02;
    if (s.doorLeft == DoorState::Opening || s.doorLeft == DoorState::Closing ||
        s.doorRight == DoorState::Opening || s.doorRight == DoorState::Closing)
        consumption += 0.03;
    if (s.mainReservoirBar < 8.4) m_compressorOn = true;
    if (s.mainReservoirBar > 9.8) m_compressorOn = false;
    s.mainReservoirBar += ((m_compressorOn ? 0.06 : 0.0) - consumption) * dt;
    s.mainReservoirBar = std::clamp(s.mainReservoirBar, 5.0, 10.5);
}

void TrainSimulation::tickMotion(double dt)
{
    TrainState &s = m_s;
    const double vMs = kmh2ms(s.speedKmh);

    // Cruise control drives the lever towards a proportional demand.
    if (s.afbEnabled && !s.brakeIntervention && !s.emergencyBrake &&
        s.sifa != SifaStage::EmergencyBrake) {
        const double target = std::min(s.afbSetKmh, s.permittedSpeedKmh);
        const double leverTarget = std::clamp((target - s.speedKmh) * 9.0, -85.0, 90.0);
        s.leverPercent = approach(s.leverPercent, leverTarget, 120.0, dt);
    }

    // Tractive effort (cut by door interlock, pantograph, any braking system).
    double tractionKn = 0.0;
    const bool tractionAllowed = s.pantographUp && !s.tractionLocked &&
                                 !s.brakeIntervention && !s.emergencyBrake &&
                                 s.sifa != SifaStage::EmergencyBrake;
    if (s.leverPercent > 0 && tractionAllowed) {
        const double fLimit = std::min(kMaxTractionKn, kMaxPowerKw / std::max(vMs, 3.0));
        tractionKn = fLimit * (s.leverPercent / 100.0);
    }

    // Brake forces (mirror of tickPneumatics blending).
    double demand = std::max(0.0, -s.leverPercent / 100.0);
    if (s.brakeIntervention) demand = std::max(demand, 0.7);
    const bool emergency = s.emergencyBrake || s.sifa == SifaStage::EmergencyBrake;
    if (emergency) demand = 1.0;
    const double wanted = (emergency ? kEmergencyBrakeKn : kServiceBrakeKn) * demand;
    const double dynFade = std::clamp(s.speedKmh / 25.0, 0.0, 1.0);
    const double dynKn = emergency ? 0.0 : std::min(wanted, kMaxDynBrakeKn * dynFade);
    const double pneuKn = s.brakeCylinderBar * kCylinderKnPerBar;

    const double resistanceKn =
        2.0 + 0.033 * s.speedKmh + 0.00048 * s.speedKmh * s.speedKmh;
    const double gradeKn = kMassKg * 9.81 * s.gradientPermille / 1000.0 / 1000.0;

    const double netKn = tractionKn - dynKn - pneuKn - resistanceKn - gradeKn;
    double accel = netKn * 1000.0 / kMassKg;
    if (vMs <= 0.0 && netKn <= 0.0) accel = 0.0; // brakes hold at standstill

    double newV = std::max(0.0, vMs + accel * dt);
    s.accelMs2 = dt > 0 ? (newV - vMs) / dt : 0.0;
    s.speedKmh = ms2kmh(newV);
    s.routePositionM = std::fmod(s.routePositionM + newV * dt, m_routeLengthM);
    s.odometerKm += newV * dt / 1000.0;

    s.tractionEffortKn = tractionKn - dynKn;
}

void TrainSimulation::tickElectric(double dt)
{
    TrainState &s = m_s;
    const double vMs = kmh2ms(s.speedKmh);
    const double t = s.simTimeS;

    const double drawnMw = std::max(0.0, s.tractionEffortKn) * vMs / 1000.0 + 0.28;
    const double regenMw = std::max(0.0, -s.tractionEffortKn) * vMs / 1000.0 * 0.85;
    s.powerMw = s.pantographUp ? drawnMw - regenMw : 0.0;

    const double vTarget = s.pantographUp
        ? 15.0 + 0.35 * std::sin(0.9 * t) + 0.2 * std::sin(0.23 * t + 1.7)
              - 0.8 * (s.powerMw / 4.2)
        : 0.0;
    s.lineVoltageKv = approach(s.lineVoltageKv, vTarget, 25.0, dt);

    s.motorCurrentA = s.lineVoltageKv > 1.0
        ? std::abs(s.powerMw) * 1000.0 / s.lineVoltageKv
        : 0.0;
    s.energyKwh += s.powerMw * 1000.0 * dt / 3600.0;
}

void TrainSimulation::tickComms(double dt)
{
    if (m_s.radio == RadioState::Calling) {
        m_radioTimerS += dt;
        if (m_radioTimerS > 2.0) m_s.radio = RadioState::Connected;
    }
}

void TrainSimulation::tickAdvisor()
{
    TrainState &s = m_s;
    if (s.speedKmh < 1.0) { s.advisor = AdvisorHint::None; return; }
    if (s.brakeIntervention || s.overspeedWarning) { s.advisor = AdvisorHint::Brake; return; }

    if (s.distanceToTargetM >= 0 && s.speedKmh > s.targetSpeedKmh + 2.0) {
        const double v = kmh2ms(s.speedKmh);
        const double vt = kmh2ms(s.targetSpeedKmh);
        const double brakeDist = (v * v - vt * vt) / (2.0 * 0.55);
        const double coastDist = (v * v - vt * vt) / (2.0 * 0.10);
        if (s.distanceToTargetM <= brakeDist) { s.advisor = AdvisorHint::Brake; return; }
        if (s.distanceToTargetM <= coastDist) { s.advisor = AdvisorHint::Coast; return; }
    }
    if (s.speedKmh < s.permittedSpeedKmh - 15.0) { s.advisor = AdvisorHint::Power; return; }
    s.advisor = AdvisorHint::Hold;
}

void TrainSimulation::rebuildAlerts()
{
    TrainState &s = m_s;
    s.alerts.clear();
    auto add = [&](AlertSeverity sev, const char *text) {
        s.alerts.push_back({sev, text});
    };

    if (s.sifa == SifaStage::EmergencyBrake) add(AlertSeverity::Critical, "SIFA PENALTY BRAKE - STOP & ACKNOWLEDGE");
    else if (s.sifa == SifaStage::AudibleWarning) add(AlertSeverity::Critical, "SIFA - ACKNOWLEDGE NOW");
    else if (s.sifa == SifaStage::VisualWarning) add(AlertSeverity::Warning, "SIFA - ACKNOWLEDGE");
    if (s.emergencyBrake) add(AlertSeverity::Critical, "EMERGENCY BRAKE APPLIED");
    if (s.brakeIntervention) add(AlertSeverity::Critical, "OVERSPEED - BRAKE INTERVENTION");
    else if (s.overspeedWarning) add(AlertSeverity::Warning, "OVERSPEED - REDUCE SPEED");
    if (s.tractionLocked) add(AlertSeverity::Warning, "TRACTION LOCKED - DOORS NOT SECURED");
    if (!s.pantographUp) add(AlertSeverity::Warning, "PANTOGRAPH LOWERED - NO LINE VOLTAGE");
    if (s.mainReservoirBar < 7.0) add(AlertSeverity::Warning, "MAIN RESERVOIR PRESSURE LOW");
    if (s.paActive) add(AlertSeverity::Info, "PA ANNOUNCEMENT LIVE");
    if (s.radio == RadioState::Calling) add(AlertSeverity::Info, "RADIO - CALLING CONTROL CENTRE");
    else if (s.radio == RadioState::Connected) add(AlertSeverity::Info, "RADIO - CONNECTED TO CONTROL CENTRE");
}

} // namespace traincore

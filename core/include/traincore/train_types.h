#ifndef TRAINCORE_TRAIN_TYPES_H
#define TRAINCORE_TRAIN_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace traincore {

// Vigilance device (dead man's handle) escalation stages.
enum class SifaStage : std::uint8_t {
    Inactive,        // train at standstill, supervision suspended
    Armed,           // supervision running, no action required yet
    VisualWarning,   // lamp on - acknowledge now
    AudibleWarning,  // lamp + horn - last chance
    EmergencyBrake   // penalty brake applied until standstill + acknowledge
};

enum class DoorState : std::uint8_t { Locked, Released, Opening, Open, Closing };

enum class RadioState : std::uint8_t { Idle, Calling, Connected };

// Driving advisor recommendation (driver advisory system).
enum class AdvisorHint : std::uint8_t { None, Power, Hold, Coast, Brake };

enum class AlertSeverity : std::uint8_t { Info, Warning, Critical };

// Every push button / switch of the cab HMI funnels through this one enum,
// which is what keeps the frontends interchangeable: a UI only ever calls
// TrainSimulation::command() / setLever() and renders TrainState.
enum class Command : std::uint8_t {
    SifaAcknowledge,
    PaToggle,           // public address to passengers
    RadioToggle,        // voice call to traffic control
    DoorsLeftRelease,
    DoorsLeftClose,
    DoorsRightRelease,
    DoorsRightClose,
    PantographToggle,
    AfbToggle,          // cruise control on/off
    AfbIncrease,        // +5 km/h set speed
    AfbDecrease,        // -5 km/h set speed
    EmergencyBrakeToggle
};

struct Alert {
    AlertSeverity severity = AlertSeverity::Info;
    std::string text;
};

// Complete snapshot of everything a cab display renders. Plain data, no Qt,
// no LVGL - the single contract between simulation core and any frontend.
struct TrainState {
    // Time / position
    double simTimeS = 0.0;          // seconds since power-up
    int clockHour = 9, clockMinute = 41, clockSecond = 0;
    double odometerKm = 128409.4;
    double routePositionM = 0.0;

    // Motion & supervision
    double speedKmh = 0.0;
    double accelMs2 = 0.0;
    double permittedSpeedKmh = 0.0; // supervised ceiling (limit + braking curve)
    double targetSpeedKmh = 0.0;    // speed at the governing target ahead
    double distanceToTargetM = 0.0; // distance to that target (<0: none)
    bool overspeedWarning = false;
    bool brakeIntervention = false; // system is braking against the driver

    // Driver controls
    double leverPercent = 0.0;      // -100 full brake .. +100 full traction
    bool emergencyBrake = false;
    bool afbEnabled = false;
    double afbSetKmh = 0.0;
    AdvisorHint advisor = AdvisorHint::None;

    // Traction & electric
    double tractionEffortKn = 0.0;  // + tractive, - electrodynamic brake
    double powerMw = 0.0;           // + drawn, - regenerated
    double lineVoltageKv = 15.0;
    double motorCurrentA = 0.0;
    double energyKwh = 0.0;         // net consumption this trip
    bool pantographUp = true;
    bool tractionLocked = false;    // door interlock active

    // Pneumatics
    double brakePipeBar = 5.0;
    double brakeCylinderBar = 0.0;
    double mainReservoirBar = 9.6;

    // Vigilance
    SifaStage sifa = SifaStage::Inactive;
    double sifaCountdownS = 0.0;    // time until next escalation

    // Doors
    DoorState doorLeft = DoorState::Locked;
    DoorState doorRight = DoorState::Locked;

    // Communication
    bool paActive = false;
    RadioState radio = RadioState::Idle;

    // Route / journey
    std::string nextStationName;
    double nextStationDistanceM = 0.0;
    double gradientPermille = 0.0;  // + climbing
    std::string serviceId = "ICE 2286";

    std::vector<Alert> alerts;      // active alerts, most severe first
};

// A change of line speed / gradient at a route position, plus station stops.
// Exposed so frontends can draw the planning strip / 3D preview themselves.
struct RouteSegment {
    double startM = 0.0;
    double limitKmh = 120.0;
    double gradientPermille = 0.0;
};

struct RouteStation {
    double positionM = 0.0;
    std::string name;
};

} // namespace traincore

#endif // TRAINCORE_TRAIN_TYPES_H

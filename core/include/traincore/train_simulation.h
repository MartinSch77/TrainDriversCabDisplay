#ifndef TRAINCORE_TRAIN_SIMULATION_H
#define TRAINCORE_TRAIN_SIMULATION_H

#include "train_types.h"

namespace traincore {

// UI-agnostic train + systems simulation. A frontend owns one instance,
// calls tick() at its own frame/timer rate, forwards operator input via
// setLever()/command(), and renders state(). Nothing else is required to
// port the HMI to a new UI technology.
class TrainSimulation {
public:
    TrainSimulation();

    // Advance the world by dtS seconds (clamped internally to keep the
    // integration stable if the host UI stalls).
    void tick(double dtS);

    // Combined power/brake lever, -100 (full service brake) .. +100 (full power).
    void setLever(double percent);

    void command(Command cmd);

    const TrainState &state() const { return m_s; }

    const std::vector<RouteSegment> &route() const { return m_route; }
    const std::vector<RouteStation> &stations() const { return m_stations; }
    double routeLengthM() const { return m_routeLengthM; }

    // Line speed limit / gradient at an arbitrary route position (wraps).
    double limitAtM(double positionM) const;
    double gradientAtM(double positionM) const;

private:
    void tickMotion(double dt);
    void tickSupervision(double dt);
    void tickSifa(double dt);
    void tickDoors(double dt);
    void tickPneumatics(double dt);
    void tickElectric(double dt);
    void tickComms(double dt);
    void tickJourney();
    void tickAdvisor();
    void rebuildAlerts();

    static void stepDoor(DoorState &door, double &timer, double dt);

    TrainState m_s;
    std::vector<RouteSegment> m_route;
    std::vector<RouteStation> m_stations;
    double m_routeLengthM = 0.0;

    // internal timers / latches
    double m_sifaTimerS = 0.0;
    bool m_sifaPenaltyLatched = false;
    double m_doorLeftTimerS = 0.0;
    double m_doorRightTimerS = 0.0;
    double m_radioTimerS = 0.0;
    double m_startClockS = 0.0;
    bool m_compressorOn = false;
    double m_interventionUntilBelowKmh = -1.0;
};

} // namespace traincore

#endif // TRAINCORE_TRAIN_SIMULATION_H

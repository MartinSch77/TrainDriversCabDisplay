#ifndef TRAINBACKEND_H
#define TRAINBACKEND_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "traincore/train_simulation.h"

// Thin QObject adapter between the UI-agnostic traincore simulation and QML.
// All values are refreshed from one snapshot per tick and published through a
// single stateChanged() notify signal (30 Hz), which QML bindings consume.
class TrainBackend : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // Motion & supervision
    Q_PROPERTY(double speedKmh READ speedKmh NOTIFY stateChanged)
    Q_PROPERTY(double accelMs2 READ accelMs2 NOTIFY stateChanged)
    Q_PROPERTY(double permittedSpeedKmh READ permittedSpeedKmh NOTIFY stateChanged)
    Q_PROPERTY(double targetSpeedKmh READ targetSpeedKmh NOTIFY stateChanged)
    Q_PROPERTY(double distanceToTargetM READ distanceToTargetM NOTIFY stateChanged)
    Q_PROPERTY(bool overspeedWarning READ overspeedWarning NOTIFY stateChanged)
    Q_PROPERTY(bool brakeIntervention READ brakeIntervention NOTIFY stateChanged)

    // Driver controls
    Q_PROPERTY(double leverPercent READ leverPercent NOTIFY stateChanged)
    Q_PROPERTY(bool emergencyBrake READ emergencyBrake NOTIFY stateChanged)
    Q_PROPERTY(bool afbEnabled READ afbEnabled NOTIFY stateChanged)
    Q_PROPERTY(double afbSetKmh READ afbSetKmh NOTIFY stateChanged)
    Q_PROPERTY(int advisor READ advisor NOTIFY stateChanged)

    // Traction & electric
    Q_PROPERTY(double tractionEffortKn READ tractionEffortKn NOTIFY stateChanged)
    Q_PROPERTY(double powerMw READ powerMw NOTIFY stateChanged)
    Q_PROPERTY(double lineVoltageKv READ lineVoltageKv NOTIFY stateChanged)
    Q_PROPERTY(double motorCurrentA READ motorCurrentA NOTIFY stateChanged)
    Q_PROPERTY(double energyKwh READ energyKwh NOTIFY stateChanged)
    Q_PROPERTY(bool pantographUp READ pantographUp NOTIFY stateChanged)
    Q_PROPERTY(bool tractionLocked READ tractionLocked NOTIFY stateChanged)

    // Pneumatics
    Q_PROPERTY(double brakePipeBar READ brakePipeBar NOTIFY stateChanged)
    Q_PROPERTY(double brakeCylinderBar READ brakeCylinderBar NOTIFY stateChanged)
    Q_PROPERTY(double mainReservoirBar READ mainReservoirBar NOTIFY stateChanged)

    // Vigilance / doors / comms
    Q_PROPERTY(int sifaStage READ sifaStage NOTIFY stateChanged)
    Q_PROPERTY(double sifaCountdownS READ sifaCountdownS NOTIFY stateChanged)
    Q_PROPERTY(int doorLeft READ doorLeft NOTIFY stateChanged)
    Q_PROPERTY(int doorRight READ doorRight NOTIFY stateChanged)
    Q_PROPERTY(bool paActive READ paActive NOTIFY stateChanged)
    Q_PROPERTY(int radioState READ radioState NOTIFY stateChanged)

    // Journey
    Q_PROPERTY(QString clockText READ clockText NOTIFY stateChanged)
    Q_PROPERTY(QString serviceId READ serviceId NOTIFY stateChanged)
    Q_PROPERTY(QString nextStationName READ nextStationName NOTIFY stateChanged)
    Q_PROPERTY(double nextStationDistanceM READ nextStationDistanceM NOTIFY stateChanged)
    Q_PROPERTY(double gradientPermille READ gradientPermille NOTIFY stateChanged)
    Q_PROPERTY(double odometerKm READ odometerKm NOTIFY stateChanged)
    Q_PROPERTY(double routePositionM READ routePositionM NOTIFY stateChanged)
    Q_PROPERTY(double routeLengthM READ routeLengthM CONSTANT)
    Q_PROPERTY(QVariantList alerts READ alerts NOTIFY stateChanged)

public:
    // QML-visible mirrors of the traincore enums (same order and values).
    enum class Sifa { Inactive, Armed, VisualWarning, AudibleWarning, EmergencyBrake };
    Q_ENUM(Sifa)
    enum class Door { Locked, Released, Opening, Open, Closing };
    Q_ENUM(Door)
    enum class Radio { Idle, Calling, Connected };
    Q_ENUM(Radio)
    enum class Hint { None, Power, Hold, Coast, Brake };
    Q_ENUM(Hint)
    enum class Cmd {
        SifaAcknowledge, PaToggle, RadioToggle,
        DoorsLeftRelease, DoorsLeftClose, DoorsRightRelease, DoorsRightClose,
        PantographToggle, AfbToggle, AfbIncrease, AfbDecrease,
        EmergencyBrakeToggle
    };
    Q_ENUM(Cmd)

    explicit TrainBackend(QObject *parent = nullptr);

    // Fully qualified for moc/QML invocation (clazy-fully-qualified-moc-types).
    Q_INVOKABLE void send(TrainBackend::Cmd cmd);
    Q_INVOKABLE void setLever(double percent);

    // Static route data for the planning strip / 3D preview.
    Q_INVOKABLE QVariantList routeProfile() const;
    Q_INVOKABLE QVariantList routeStations() const;

    double speedKmh() const { return m_sim.state().speedKmh; }
    double accelMs2() const { return m_sim.state().accelMs2; }
    double permittedSpeedKmh() const { return m_sim.state().permittedSpeedKmh; }
    double targetSpeedKmh() const { return m_sim.state().targetSpeedKmh; }
    double distanceToTargetM() const { return m_sim.state().distanceToTargetM; }
    bool overspeedWarning() const { return m_sim.state().overspeedWarning; }
    bool brakeIntervention() const { return m_sim.state().brakeIntervention; }
    double leverPercent() const { return m_sim.state().leverPercent; }
    bool emergencyBrake() const { return m_sim.state().emergencyBrake; }
    bool afbEnabled() const { return m_sim.state().afbEnabled; }
    double afbSetKmh() const { return m_sim.state().afbSetKmh; }
    int advisor() const { return static_cast<int>(m_sim.state().advisor); }
    double tractionEffortKn() const { return m_sim.state().tractionEffortKn; }
    double powerMw() const { return m_sim.state().powerMw; }
    double lineVoltageKv() const { return m_sim.state().lineVoltageKv; }
    double motorCurrentA() const { return m_sim.state().motorCurrentA; }
    double energyKwh() const { return m_sim.state().energyKwh; }
    bool pantographUp() const { return m_sim.state().pantographUp; }
    bool tractionLocked() const { return m_sim.state().tractionLocked; }
    double brakePipeBar() const { return m_sim.state().brakePipeBar; }
    double brakeCylinderBar() const { return m_sim.state().brakeCylinderBar; }
    double mainReservoirBar() const { return m_sim.state().mainReservoirBar; }
    int sifaStage() const { return static_cast<int>(m_sim.state().sifa); }
    double sifaCountdownS() const { return m_sim.state().sifaCountdownS; }
    int doorLeft() const { return static_cast<int>(m_sim.state().doorLeft); }
    int doorRight() const { return static_cast<int>(m_sim.state().doorRight); }
    bool paActive() const { return m_sim.state().paActive; }
    int radioState() const { return static_cast<int>(m_sim.state().radio); }
    QString clockText() const;
    QString serviceId() const { return QString::fromStdString(m_sim.state().serviceId); }
    QString nextStationName() const { return QString::fromStdString(m_sim.state().nextStationName); }
    double nextStationDistanceM() const { return m_sim.state().nextStationDistanceM; }
    double gradientPermille() const { return m_sim.state().gradientPermille; }
    double odometerKm() const { return m_sim.state().odometerKm; }
    double routePositionM() const { return m_sim.state().routePositionM; }
    double routeLengthM() const { return m_sim.routeLengthM(); }
    QVariantList alerts() const;

signals:
    void stateChanged();

private:
    traincore::TrainSimulation m_sim;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
};

#endif // TRAINBACKEND_H

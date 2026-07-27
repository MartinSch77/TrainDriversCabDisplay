import RailDeck
import QtQuick
import QtQuick.Layouts

// RailDeck Pro - driver's cab display, Qt Quick frontend.
// 1280x800, dark cab theme, all state from the traincore simulation backend.
Window {
    id: win

    width: 1280
    height: 800
    minimumWidth: 1280
    minimumHeight: 800
    visible: true
    title: "RailDeck Pro — Cab Display"
    color: Theme.bg

    TrainBackend { id: backend }

    function doorText(state) {
        return ["LOCKED", "RELEASED", "OPENING", "OPEN", "CLOSING"][state]
    }
    readonly property bool standstill: backend.speedKmh < 0.5

    // ------- keyboard controls -------
    Shortcut { sequence: "Space"; onActivated: backend.send(TrainBackend.Cmd.SifaAcknowledge) }
    Shortcut { sequence: "Up"; onActivated: backend.setLever(backend.leverPercent + 10) }
    Shortcut { sequence: "W"; onActivated: backend.setLever(backend.leverPercent + 10) }
    Shortcut { sequence: "Down"; onActivated: backend.setLever(backend.leverPercent - 10) }
    Shortcut { sequence: "S"; onActivated: backend.setLever(backend.leverPercent - 10) }
    Shortcut { sequence: "X"; onActivated: backend.setLever(0) }
    Shortcut { sequence: "Q"; onActivated: backend.send(TrainBackend.Cmd.PaToggle) }
    Shortcut { sequence: "C"; onActivated: backend.send(TrainBackend.Cmd.RadioToggle) }
    Shortcut { sequence: "T"; onActivated: backend.send(TrainBackend.Cmd.PantographToggle) }
    Shortcut { sequence: "A"; onActivated: backend.send(TrainBackend.Cmd.AfbToggle) }
    Shortcut { sequence: "+"; onActivated: backend.send(TrainBackend.Cmd.AfbIncrease) }
    Shortcut { sequence: "-"; onActivated: backend.send(TrainBackend.Cmd.AfbDecrease) }
    Shortcut { sequence: "E"; onActivated: backend.send(TrainBackend.Cmd.EmergencyBrakeToggle) }
    Shortcut { sequence: "1"; onActivated: backend.send(TrainBackend.Cmd.DoorsLeftRelease) }
    Shortcut { sequence: "2"; onActivated: backend.send(TrainBackend.Cmd.DoorsLeftClose) }
    Shortcut { sequence: "3"; onActivated: backend.send(TrainBackend.Cmd.DoorsRightRelease) }
    Shortcut { sequence: "4"; onActivated: backend.send(TrainBackend.Cmd.DoorsRightClose) }

    StatusBar {
        id: statusBar
        backend: backend
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.headerHeight
    }

    // ------- main instrument area -------
    RowLayout {
        anchors.top: statusBar.bottom
        anchors.bottom: buttonBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        spacing: 10

        ColumnLayout { // pneumatics
            Layout.preferredWidth: 225
            Layout.fillHeight: true
            spacing: 10

            ArcGauge {
                Layout.fillWidth: true
                title: "BRAKE PIPE"; unit: "bar"
                value: backend.brakePipeBar; from: 0; to: 6; majorTicks: 6
                zones: [
                    { from: 0.0, to: 3.2, color: Theme.danger },
                    { from: 4.6, to: 5.2, color: Theme.ok }
                ]
            }
            ArcGauge {
                Layout.fillWidth: true
                title: "BRAKE CYLINDER"; unit: "bar"
                value: backend.brakeCylinderBar; from: 0; to: 5; majorTicks: 5
                zones: [{ from: 3.8, to: 5.0, color: Theme.warn }]
            }
            ArcGauge {
                Layout.fillWidth: true
                title: "MAIN RESERVOIR"; unit: "bar"
                value: backend.mainReservoirBar; from: 0; to: 11; majorTicks: 11
                zones: [
                    { from: 0.0, to: 6.5, color: Theme.danger },
                    { from: 8.0, to: 10.0, color: Theme.ok }
                ]
            }
            InfoTile {
                Layout.fillWidth: true
                title: "ACCELERATION"
                value: (backend.accelMs2 >= 0 ? "+" : "") + backend.accelMs2.toFixed(2)
                unit: "m/s²"
                valueColor: backend.accelMs2 >= 0 ? Theme.textPrimary : Theme.warn
            }
            Item { Layout.fillHeight: true }
        }

        ColumnLayout { // speed + driving
            Layout.preferredWidth: 500
            Layout.fillHeight: true
            spacing: 10

            Speedometer {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 470
                Layout.preferredHeight: 470
                speed: backend.speedKmh
                permitted: backend.permittedSpeedKmh
                target: backend.targetSpeedKmh
                distanceToTarget: backend.distanceToTargetM
                warning: backend.overspeedWarning
                intervention: backend.brakeIntervention
                afbEnabled: backend.afbEnabled
                afbSet: backend.afbSetKmh
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 10
                AdvisorPanel { hint: backend.advisor }
                LeverControl {
                    id: lever
                    afbControlled: backend.afbEnabled
                    onMoved: (percent) => backend.setLever(percent)
                    Connections {
                        target: backend
                        function onStateChanged() { lever.externalSync(backend.leverPercent) }
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }

        ColumnLayout { // traction & electric
            Layout.preferredWidth: 225
            Layout.fillHeight: true
            spacing: 10

            ArcGauge {
                Layout.fillWidth: true
                title: "LINE VOLTAGE"; unit: "kV"
                value: backend.lineVoltageKv; from: 0; to: 18; majorTicks: 6
                zones: [
                    { from: 0.0, to: 11.0, color: Theme.danger },
                    { from: 13.5, to: 16.5, color: Theme.ok }
                ]
            }
            ArcGauge {
                Layout.fillWidth: true
                title: "MOTOR CURRENT"; unit: "A"
                value: backend.motorCurrentA; from: 0; to: 400; majorTicks: 8; decimals: 0
                zones: [{ from: 320, to: 400, color: Theme.warn }]
            }
            EffortBar {
                Layout.fillWidth: true
                value: backend.tractionEffortKn
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                InfoTile {
                    Layout.fillWidth: true
                    title: "POWER"
                    value: (backend.powerMw >= 0 ? "" : "−") + Math.abs(backend.powerMw).toFixed(2)
                    unit: "MW"
                    valueColor: backend.powerMw >= 0 ? Theme.traction : Theme.regen
                }
                InfoTile {
                    Layout.fillWidth: true
                    title: "ENERGY"
                    value: backend.energyKwh.toFixed(0)
                    unit: "kWh"
                }
            }
            Item { Layout.fillHeight: true }
        }

        ColumnLayout { // route preview
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            spacing: 10

            TrackView3D {
                Layout.fillWidth: true
                Layout.preferredHeight: 240
                speedKmh: backend.speedKmh
                routePositionM: backend.routePositionM
                gradientPermille: backend.gradientPermille
                distanceToTargetM: backend.distanceToTargetM
            }
            PlanningStrip {
                Layout.fillWidth: true
                Layout.fillHeight: true
                backend: backend
            }
        }
    }

    // ------- button bar -------
    Rectangle {
        id: buttonBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 96
        color: Theme.panelInset
        border.color: Theme.line
        border.width: Theme.borderWidth

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            CabButton { // vigilance / dead man's acknowledge
                Layout.preferredWidth: 140
                Layout.preferredHeight: 72
                label: "SIFA"
                sub: backend.sifaStage === 0 ? "STANDBY"
                     : backend.sifaStage === 1 ? Math.ceil(backend.sifaCountdownS) + " s"
                     : backend.sifaStage === 4 ? "PENALTY BRAKE"
                     : "ACK NOW · " + Math.ceil(backend.sifaCountdownS) + " s"
                active: backend.sifaStage >= 2
                alarm: backend.sifaStage >= 2
                activeColor: Theme.danger
                onClicked: backend.send(TrainBackend.Cmd.SifaAcknowledge)
            }

            CabButton {
                Layout.preferredHeight: 72
                label: "PA"
                sub: backend.paActive ? "LIVE" : "PASSENGERS"
                active: backend.paActive
                onClicked: backend.send(TrainBackend.Cmd.PaToggle)
            }
            CabButton {
                Layout.preferredHeight: 72
                label: "RADIO"
                sub: ["CONTROL CTR", "CALLING…", "ONLINE"][backend.radioState]
                active: backend.radioState !== 0
                onClicked: backend.send(TrainBackend.Cmd.RadioToggle)
            }

            Rectangle { width: 1; Layout.preferredHeight: 56; color: Theme.line }

            CabButton {
                Layout.preferredWidth: 104
                Layout.preferredHeight: 72
                label: "L RELEASE"
                sub: win.doorText(backend.doorLeft)
                active: backend.doorLeft !== 0
                activeColor: Theme.doorOpen
                enabled: win.standstill || backend.doorLeft !== 0
                onClicked: backend.send(TrainBackend.Cmd.DoorsLeftRelease)
            }
            CabButton {
                Layout.preferredWidth: 104
                Layout.preferredHeight: 72
                label: "L CLOSE"
                sub: "DOORS LEFT"
                activeColor: Theme.doorClosed
                onClicked: backend.send(TrainBackend.Cmd.DoorsLeftClose)
            }
            CabButton {
                Layout.preferredWidth: 104
                Layout.preferredHeight: 72
                label: "R RELEASE"
                sub: win.doorText(backend.doorRight)
                active: backend.doorRight !== 0
                activeColor: Theme.doorOpen
                enabled: win.standstill || backend.doorRight !== 0
                onClicked: backend.send(TrainBackend.Cmd.DoorsRightRelease)
            }
            CabButton {
                Layout.preferredWidth: 104
                Layout.preferredHeight: 72
                label: "R CLOSE"
                sub: "DOORS RIGHT"
                activeColor: Theme.doorClosed
                onClicked: backend.send(TrainBackend.Cmd.DoorsRightClose)
            }

            Rectangle { width: 1; Layout.preferredHeight: 56; color: Theme.line }

            CabButton {
                Layout.preferredHeight: 72
                label: "PANTO"
                sub: backend.pantographUp ? "UP" : "DOWN"
                active: backend.pantographUp
                activeColor: Theme.ok
                onClicked: backend.send(TrainBackend.Cmd.PantographToggle)
            }
            CabButton {
                Layout.preferredHeight: 72
                label: "AFB"
                sub: backend.afbEnabled ? Math.round(backend.afbSetKmh) + " km/h" : "CRUISE"
                active: backend.afbEnabled
                onClicked: backend.send(TrainBackend.Cmd.AfbToggle)
            }
            CabButton {
                Layout.preferredWidth: 52
                Layout.preferredHeight: 72
                label: "−"
                onClicked: backend.send(TrainBackend.Cmd.AfbDecrease)
            }
            CabButton {
                Layout.preferredWidth: 52
                Layout.preferredHeight: 72
                label: "+"
                onClicked: backend.send(TrainBackend.Cmd.AfbIncrease)
            }

            Item { Layout.fillWidth: true }

            CabButton {
                Layout.preferredWidth: 120
                Layout.preferredHeight: 72
                label: "EMERG"
                sub: backend.emergencyBrake ? "APPLIED" : "BRAKE"
                active: backend.emergencyBrake
                alarm: backend.emergencyBrake
                activeColor: Theme.danger
                onClicked: backend.send(TrainBackend.Cmd.EmergencyBrakeToggle)
            }
        }
    }
}

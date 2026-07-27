import RailDeck
import QtQuick

// Top strip: service, driving mode, active alert ticker, next stop, clock
// and system badges (pantograph / radio / PA).
Rectangle {
    id: root

    required property var backend

    readonly property var alertList: backend.alerts
    property int alertIndex: 0

    color: Theme.panelInset
    border.color: Theme.line
    border.width: Theme.borderWidth

    Timer { // cycle through concurrent alerts
        interval: 3000; running: root.alertList.length > 1; repeat: true
        onTriggered: root.alertIndex = (root.alertIndex + 1) % root.alertList.length
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Rectangle {
            width: serviceText.width + 20; height: 28; radius: Theme.radiusSmall
            color: Theme.panel
            border.color: Theme.accent
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: serviceText
                anchors.centerIn: parent
                text: root.backend.serviceId
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeBase
                font.bold: true
            }
        }

        Rectangle {
            width: modeText.width + 20; height: 28; radius: Theme.radiusSmall
            anchors.verticalCenter: parent.verticalCenter
            color: root.backend.emergencyBrake ? Theme.dangerDim
                   : root.backend.afbEnabled ? Theme.buttonFaceActive : Theme.panel
            border.color: root.backend.emergencyBrake ? Theme.danger
                          : root.backend.afbEnabled ? Theme.accent : Theme.line
            border.width: 1
            Text {
                id: modeText
                anchors.centerIn: parent
                text: root.backend.emergencyBrake ? "EMERGENCY"
                      : root.backend.afbEnabled
                        ? "AFB " + Math.round(root.backend.afbSetKmh) : "MANUAL"
                color: root.backend.emergencyBrake ? Theme.danger
                       : root.backend.afbEnabled ? Theme.accent : Theme.textSecondary
                font.pixelSize: Theme.fontSizeBase
                font.bold: true
            }
        }
    }

    // Alert ticker, centred
    Rectangle {
        anchors.centerIn: parent
        width: 440; height: 30; radius: Theme.radiusSmall
        color: root.alertList.length === 0 ? "transparent"
               : root.alertList[Math.min(root.alertIndex, root.alertList.length - 1)].severity === 2
                 ? Theme.dangerDim : Theme.panel
        border.color: root.alertList.length === 0 ? "transparent"
               : [Theme.line, Theme.warn, Theme.danger]
                 [root.alertList[Math.min(root.alertIndex, root.alertList.length - 1)].severity]
        border.width: 1

        Text {
            anchors.centerIn: parent
            width: parent.width - 16
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: root.alertList.length === 0 ? ""
                  : root.alertList[Math.min(root.alertIndex, root.alertList.length - 1)].text
            color: root.alertList.length === 0 ? Theme.textDim
                   : [Theme.textSecondary, Theme.warn, Theme.danger]
                     [root.alertList[Math.min(root.alertIndex, root.alertList.length - 1)].severity]
            font.pixelSize: Theme.fontSizeBase
            font.bold: root.alertList.length > 0 && root.alertList[Math.min(root.alertIndex, root.alertList.length - 1)].severity > 0

            SequentialAnimation on opacity {
                running: root.alertList.length > 0
                         && root.alertList[Math.min(root.alertIndex, root.alertList.length - 1)].severity === 2
                loops: Animation.Infinite
                alwaysRunToEnd: true
                NumberAnimation { to: 0.35; duration: 350 }
                NumberAnimation { to: 1.0; duration: 350 }
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        component MiniBadge: Rectangle {
            property string label: ""
            property bool on: false
            property color onColor: Theme.ok
            width: 40; height: 24; radius: Theme.radiusSmall
            color: on ? Qt.darker(onColor, 3.2) : Theme.panel
            border.color: on ? onColor : Theme.line
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            Text {
                anchors.centerIn: parent
                text: parent.label
                color: parent.on ? parent.onColor : Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
            }
        }

        MiniBadge { label: "PAN"; on: root.backend.pantographUp; onColor: Theme.ok }
        MiniBadge {
            label: "RAD"
            on: root.backend.radioState !== 0
            onColor: root.backend.radioState === 1 ? Theme.yellow : Theme.accent
        }
        MiniBadge { label: "PA"; on: root.backend.paActive; onColor: Theme.accent }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "→ " + root.backend.nextStationName + " · "
                  + (root.backend.nextStationDistanceM / 1000).toFixed(1) + " km"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBase
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.backend.clockText
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMedium
            font.family: "DejaVu Sans Mono"
            font.bold: true
        }
    }
}

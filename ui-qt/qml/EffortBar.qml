import RailDeck
import QtQuick

// Bipolar tractive-effort bar: traction upwards (cyan), electrodynamic
// braking downwards (green, regenerating).
Item {
    id: root

    property real value: 0          // kN, +traction / -braking
    property real range: 250        // +- kN

    implicitWidth: 158
    implicitHeight: 190

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.panel
        border.color: Theme.line
        border.width: Theme.borderWidth
    }

    Text {
        id: titleText
        anchors.top: parent.top
        anchors.topMargin: 7
        anchors.horizontalCenter: parent.horizontalCenter
        text: "EFFORT"
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }

    Item {
        id: track
        anchors.top: titleText.bottom
        anchors.topMargin: 8
        anchors.bottom: readout.top
        anchors.bottomMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        width: 26

        readonly property real half: height / 2
        readonly property real frac: Math.max(-1, Math.min(1, root.value / root.range))

        Rectangle {
            anchors.fill: parent
            radius: 5
            color: Theme.gaugeTrack
        }

        Rectangle { // filled amount from center
            x: 0
            width: parent.width
            y: track.frac >= 0 ? track.half - track.frac * track.half : track.half
            height: Math.abs(track.frac) * track.half
            radius: 5
            color: track.frac >= 0 ? Theme.traction : Theme.regen
            Behavior on height { NumberAnimation { duration: 100 } }
        }

        Rectangle { // zero line
            y: track.half - 1
            width: parent.width
            height: 2
            color: Theme.textSecondary
        }

        Repeater { // side ticks every 50 kN
            model: 11
            Rectangle {
                x: -8
                y: track.height * index / 10 - 1
                width: 6; height: index === 5 ? 2 : 1
                color: Theme.gaugeTick
            }
        }
    }

    Text {
        anchors.right: track.left
        anchors.rightMargin: 12
        anchors.top: track.top
        text: "kN"
        color: Theme.textDim
        font.pixelSize: Theme.fontSizeSmall
    }

    Text {
        id: readout
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        text: (root.value >= 0 ? "+" : "") + root.value.toFixed(0)
        color: root.value >= 0 ? Theme.traction : Theme.regen
        font.pixelSize: Theme.fontSizeLarge
        font.family: "DejaVu Sans Mono"
        font.bold: true
    }
}

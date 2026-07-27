import RailDeck
import QtQuick
import QtQuick.Shapes

// Small round instrument: 240-degree scale, colored zones, needle, digital value.
Item {
    id: root

    property string title: ""
    property string unit: ""
    property real value: 0
    property real from: 0
    property real to: 10
    property int decimals: 1
    property int majorTicks: 6
    property var zones: []   // [{from: real, to: real, color: color}]

    implicitWidth: 158
    implicitHeight: 172

    readonly property real startAngle: 150
    readonly property real sweep: 240

    function angleFor(v) {
        const t = Math.max(0, Math.min(1, (v - from) / (to - from)))
        return startAngle + t * sweep
    }

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
        text: root.title
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }

    Item {
        id: dial
        anchors.top: titleText.bottom
        anchors.topMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 26, parent.height - titleText.height - 22)
        height: width

        readonly property real r: width / 2 - 5

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: Theme.gaugeTrack
                strokeWidth: 7
                fillColor: "transparent"
                capStyle: ShapePath.FlatCap
                PathAngleArc {
                    centerX: dial.width / 2; centerY: dial.height / 2
                    radiusX: dial.r; radiusY: dial.r
                    startAngle: root.startAngle
                    sweepAngle: root.sweep
                }
            }
        }

        Repeater {
            model: root.zones
            Shape {
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeColor: modelData.color
                    strokeWidth: 7
                    fillColor: "transparent"
                    capStyle: ShapePath.FlatCap
                    PathAngleArc {
                        centerX: dial.width / 2; centerY: dial.height / 2
                        radiusX: dial.r; radiusY: dial.r
                        startAngle: root.angleFor(modelData.from)
                        sweepAngle: root.angleFor(modelData.to) - root.angleFor(modelData.from)
                    }
                }
            }
        }

        Repeater {
            model: root.majorTicks + 1
            Item {
                anchors.fill: parent
                rotation: root.angleFor(root.from + index * (root.to - root.from) / root.majorTicks) - 270
                Rectangle {
                    x: parent.width / 2 - 1
                    y: 1
                    width: 2; height: 7
                    color: Theme.gaugeTick
                }
            }
        }

        Item { // needle
            anchors.fill: parent
            rotation: root.angleFor(root.value) - 270
            Behavior on rotation { NumberAnimation { duration: 120 } }
            Rectangle {
                x: parent.width / 2 - 1.5
                y: 9
                width: 3
                height: dial.r * 0.52
                radius: 1.5
                color: Theme.needle
            }
        }

        Rectangle { // hub
            anchors.centerIn: parent
            width: 8; height: 8; radius: 4
            color: Theme.lineBright
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -2
            text: root.value.toFixed(root.decimals)
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLarge
            font.family: "DejaVu Sans Mono"
            font.bold: true
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -16
            text: root.unit
            color: Theme.textDim
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}

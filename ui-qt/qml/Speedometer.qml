import RailDeck
import QtQuick
import QtQuick.Shapes

// Main circular speed gauge with braking-curve supervision ring:
// grey = permitted range, yellow = braking-curve indication towards the
// target, orange = overspeed warning band, red = intervention. Needle,
// large digital readout, target/distance info and cruise set-speed marker.
Item {
    id: root

    property real speed: 0
    property real permitted: 160
    property real target: 160
    property real distanceToTarget: -1
    property bool warning: false
    property bool intervention: false
    property bool afbEnabled: false
    property real afbSet: 0

    readonly property real maxKmh: Theme.speedoMaxKmh
    readonly property real startAngle: Theme.speedoStartDeg
    readonly property real sweep: Theme.speedoSweepDeg
    readonly property bool hasTarget: distanceToTarget >= 0 && target < permitted - 0.5

    implicitWidth: 470
    implicitHeight: 470

    function angleFor(v) {
        const t = Math.max(0, Math.min(1, v / maxKmh))
        return startAngle + t * sweep
    }
    function radial(angleDeg, r) {
        const a = angleDeg * Math.PI / 180
        return Qt.point(width / 2 + r * Math.cos(a), height / 2 + r * Math.sin(a))
    }

    readonly property real ringR: width / 2 - 16
    readonly property color speedColor: intervention ? Theme.danger
                                       : warning ? Theme.warn : Theme.textPrimary

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.panel
        border.color: Theme.line
        border.width: Theme.borderWidth
    }

    // --- supervision ring -------------------------------------------------
    component RingArc: Shape {
        property real fromV: 0
        property real toV: 0
        property alias arcColor: path.strokeColor
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        visible: toV > fromV + 0.1
        ShapePath {
            id: path
            strokeWidth: 10
            fillColor: "transparent"
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.ringR; radiusY: root.ringR
                startAngle: root.angleFor(fromV)
                sweepAngle: root.angleFor(toV) - root.angleFor(fromV)
            }
        }
    }

    RingArc { fromV: 0; toV: root.maxKmh; arcColor: Theme.gaugeTrack }
    RingArc { fromV: 0; toV: root.permitted; arcColor: Theme.arcPermitted }
    RingArc { // braking-curve indication segment
        visible: root.hasTarget
        fromV: root.target; toV: root.permitted; arcColor: Theme.arcTarget
    }
    RingArc { // overspeed warning band
        visible: root.warning && !root.intervention
        fromV: root.permitted; toV: Math.min(root.permitted + 8, root.maxKmh)
        arcColor: Theme.arcWarning
    }
    RingArc { // intervention band up to actual speed
        visible: root.intervention
        fromV: root.permitted; toV: Math.max(root.speed, root.permitted + 4)
        arcColor: Theme.arcDanger
    }

    // --- scale ------------------------------------------------------------
    Repeater { // minor ticks every 10
        model: Math.floor(root.maxKmh / 10) + 1
        Item {
            anchors.fill: parent
            rotation: root.angleFor(index * 10) - 270
            Rectangle {
                x: parent.width / 2 - 1
                y: 26
                width: index % 2 === 0 ? 3 : 2
                height: index % 2 === 0 ? 16 : 9
                color: index % 2 === 0 ? Theme.gaugeTick : Theme.gaugeTrack
            }
        }
    }
    Repeater { // labels every 20
        model: Math.floor(root.maxKmh / 20) + 1
        Text {
            readonly property point p: root.radial(root.angleFor(index * 20), root.ringR - 46)
            x: p.x - width / 2
            y: p.y - height / 2
            text: index * 20
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBase
            font.family: "DejaVu Sans Mono"
        }
    }

    // --- cruise set-speed marker -------------------------------------------
    Rectangle {
        visible: root.afbEnabled
        readonly property point p: root.radial(root.angleFor(root.afbSet), root.ringR - 22)
        x: p.x - 5; y: p.y - 5
        width: 10; height: 10; radius: 5
        color: Theme.accent
        border.color: Theme.bg
        border.width: 1
    }

    // --- needle -------------------------------------------------------------
    Item {
        anchors.fill: parent
        rotation: root.angleFor(root.speed) - 270
        Behavior on rotation { NumberAnimation { duration: 120 } }
        Rectangle {
            x: parent.width / 2 - 3
            y: 34
            width: 6
            height: root.ringR - 90
            radius: 3
            color: root.speedColor
        }
    }

    // --- hub with digital readout --------------------------------------------
    Rectangle {
        anchors.centerIn: parent
        width: 190; height: 190; radius: 95
        color: Theme.panelInset
        border.color: Theme.line
        border.width: Theme.borderWidth

        Column {
            anchors.centerIn: parent
            spacing: -6
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Math.round(root.speed)
                color: root.speedColor
                font.pixelSize: Theme.fontSizeSpeed
                font.family: "DejaVu Sans Mono"
                font.bold: true
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "km/h"
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeBase
            }
        }
    }

    // --- target / distance readout -------------------------------------------
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 46
        width: 176; height: 40; radius: Theme.radiusSmall
        color: Theme.panelInset
        border.color: root.hasTarget ? Theme.arcTarget : Theme.line
        border.width: Theme.borderWidth
        visible: root.hasTarget

        Row {
            anchors.centerIn: parent
            spacing: 10
            Text {
                text: Math.round(root.target)
                color: Theme.arcTarget
                font.pixelSize: Theme.fontSizeXL
                font.family: "DejaVu Sans Mono"
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle { width: 1; height: 24; color: Theme.line; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: root.distanceToTarget >= 1000
                      ? (root.distanceToTarget / 1000).toFixed(1) + " km"
                      : Math.round(root.distanceToTarget / 10) * 10 + " m"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeMedium
                font.family: "DejaVu Sans Mono"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // permitted speed readout, top of gauge gap
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 18
        text: "PERM " + Math.round(root.permitted)
        color: Theme.arcPermitted
        font.pixelSize: Theme.fontSizeSmall
        font.family: "DejaVu Sans Mono"
        font.letterSpacing: 1
    }
}

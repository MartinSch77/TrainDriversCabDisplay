import RailDeck
import QtQuick

// Combined power/brake lever (master controller), -100 .. +100 %.
// Left of centre: service brake, right of centre: traction.
// Follows the backend while the cruise control (AFB) drives it.
Rectangle {
    id: root

    property real value: 0          // display value, synced with backend
    property bool afbControlled: false
    signal moved(real percent)

    implicitWidth: 380
    implicitHeight: 122
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: Theme.borderWidth

    function externalSync(v) {
        if (!mouse.pressed)
            root.value = v
    }

    Text {
        anchors.top: parent.top
        anchors.topMargin: 7
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.afbControlled ? "MASTER CONTROLLER — AFB" : "MASTER CONTROLLER"
        color: root.afbControlled ? Theme.accent : Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }

    Item {
        id: track
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 8
        width: parent.width - 56
        height: 34

        function xFor(v) { return (v + 100) / 200 * width }
        function vFor(x) { return Math.max(-100, Math.min(100, x / width * 200 - 100)) }

        Rectangle { // groove
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 10
            radius: 5
            color: Theme.gaugeTrack
        }

        Rectangle { // brake side fill
            anchors.verticalCenter: parent.verticalCenter
            x: track.xFor(Math.min(root.value, 0))
            width: track.xFor(0) - x
            height: 10
            radius: 5
            color: Theme.warn
            visible: root.value < 0
        }
        Rectangle { // traction side fill
            anchors.verticalCenter: parent.verticalCenter
            x: track.xFor(0)
            width: track.xFor(Math.max(root.value, 0)) - x
            height: 10
            radius: 5
            color: Theme.traction
            visible: root.value > 0
        }

        Rectangle { // centre detent
            anchors.verticalCenter: parent.verticalCenter
            x: track.xFor(0) - 1
            width: 2; height: 22
            color: Theme.textSecondary
        }

        Rectangle { // handle
            id: handle
            anchors.verticalCenter: parent.verticalCenter
            x: track.xFor(root.value) - width / 2
            width: 18; height: 34
            radius: 6
            color: root.afbControlled ? Theme.accentDim : Theme.textPrimary
            border.color: root.afbControlled ? Theme.accent : Theme.lineBright
            border.width: 1
            Behavior on x { enabled: !mouse.pressed; NumberAnimation { duration: 80 } }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            enabled: !root.afbControlled
            onPressed: (event) => { root.value = track.vFor(event.x); root.moved(root.value) }
            onPositionChanged: (event) => {
                if (pressed) { root.value = track.vFor(event.x); root.moved(root.value) }
            }
            onReleased: {
                if (Math.abs(root.value) < 8) { root.value = 0; root.moved(0) }
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        text: "BRAKE"
        color: Theme.warn
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }
    Text {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        text: "TRACTION"
        color: Theme.traction
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        text: (root.value > 0 ? "+" : "") + Math.round(root.value) + " %"
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeBase
        font.family: "DejaVu Sans Mono"
        font.bold: true
    }
}

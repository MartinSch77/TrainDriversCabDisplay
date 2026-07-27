import RailDeck
import QtQuick

// Standard cab push button: label + state line, latched "active" look,
// optional pulsing alarm state.
Rectangle {
    id: root

    property string label: ""
    property string sub: ""
    property bool active: false
    property bool alarm: false
    property color activeColor: Theme.accent
    signal clicked()

    implicitWidth: 96
    implicitHeight: Theme.buttonHeight
    radius: Theme.radius
    color: !enabled ? Theme.panelInset
           : mouse.pressed ? Qt.lighter(Theme.buttonFace, 1.4)
           : active ? Theme.buttonFaceActive : Theme.buttonFace
    border.color: alarm ? Theme.danger
                  : active ? activeColor : Theme.buttonBorder
    border.width: active || alarm ? 2 : Theme.borderWidth
    opacity: enabled ? 1.0 : 0.45

    SequentialAnimation on border.color {
        running: root.alarm
        loops: Animation.Infinite
        alwaysRunToEnd: true
        ColorAnimation { to: Theme.sifaPulse; duration: 300 }
        ColorAnimation { to: Theme.buttonBorder; duration: 300 }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            color: root.active ? root.activeColor : Theme.textPrimary
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
            font.letterSpacing: 1
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.sub.length > 0
            text: root.sub
            color: root.alarm ? Theme.danger
                   : root.active ? root.activeColor : Theme.textDim
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: root.clicked()
    }
}

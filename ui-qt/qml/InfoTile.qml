import RailDeck
import QtQuick

// Compact numeric readout tile: title on top, monospaced value + unit below.
Rectangle {
    id: root

    property string title: ""
    property string value: ""
    property string unit: ""
    property color valueColor: Theme.textPrimary

    implicitWidth: 110
    implicitHeight: 58
    radius: Theme.radiusSmall
    color: Theme.panelInset
    border.color: Theme.line
    border.width: Theme.borderWidth

    Text {
        anchors.top: parent.top
        anchors.topMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.title
        color: Theme.textDim
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4
        Text {
            id: valueText
            text: root.value
            color: root.valueColor
            font.pixelSize: Theme.fontSizeMedium
            font.family: "DejaVu Sans Mono"
            font.bold: true
        }
        Text {
            anchors.baseline: valueText.baseline
            text: root.unit
            color: Theme.textDim
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}

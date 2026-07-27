import RailDeck
import QtQuick

// Driver advisory system hint: the most energy-efficient action right now.
Rectangle {
    id: root

    property int hint: 0   // TrainBackend.Hint: None, Power, Hold, Coast, Brake

    readonly property var glyphs: ["", "▲", "■", "▼", "▼"]
    readonly property var labels: ["", "POWER", "HOLD", "COAST", "BRAKE"]
    readonly property var colors: [Theme.textDim, Theme.accent, Theme.textSecondary,
                                   Theme.yellow, Theme.warn]

    implicitWidth: 110
    implicitHeight: 122
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: Theme.borderWidth

    Text {
        anchors.top: parent.top
        anchors.topMargin: 7
        anchors.horizontalCenter: parent.horizontalCenter
        text: "ADVISOR"
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 8
        spacing: 0
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.hint > 0 ? root.glyphs[root.hint] : "–"
            color: root.colors[root.hint]
            font.pixelSize: 34
            font.bold: true
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.hint > 0 ? root.labels[root.hint] : "IDLE"
            color: root.colors[root.hint]
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
            font.letterSpacing: 2
        }
    }
}

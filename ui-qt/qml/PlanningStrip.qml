import RailDeck
import QtQuick

// Route planning area: the next 4 km ahead as a vertical strip, showing
// speed-limit changes, station stops and the supervised braking target.
Rectangle {
    id: root

    required property var backend
    readonly property real windowM: 4000

    property var events: []

    implicitWidth: 250
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: Theme.borderWidth

    Component.onCompleted: {
        const list = []
        const profile = backend.routeProfile()
        for (let i = 0; i < profile.length; i++) {
            list.push({ absM: profile[i].startM, kind: "limit",
                        label: Math.round(profile[i].limitKmh).toString() })
        }
        const sts = backend.routeStations()
        for (let j = 0; j < sts.length; j++)
            list.push({ absM: sts[j].positionM, kind: "station", label: sts[j].name })
        events = list
    }

    function yFor(dAheadM) {
        return strip.height * (1 - dAheadM / windowM)
    }

    Text {
        id: titleText
        anchors.top: parent.top
        anchors.topMargin: 7
        anchors.horizontalCenter: parent.horizontalCenter
        text: "ROUTE · 4 KM AHEAD"
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }

    Item {
        id: strip
        anchors.top: titleText.bottom
        anchors.topMargin: 10
        anchors.bottom: gradTile.top
        anchors.bottomMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 44
        anchors.right: parent.right
        anchors.rightMargin: 12

        Rectangle {
            anchors.fill: parent
            color: Theme.panelInset
            radius: Theme.radiusSmall
            border.color: Theme.line
            border.width: Theme.borderWidth
        }

        Repeater { // distance ruler
            model: 5
            Item {
                y: root.yFor(index * 1000)
                Rectangle {
                    x: 0; y: -1
                    width: strip.width; height: 1
                    color: Theme.line
                }
                Text {
                    x: -34; y: -8
                    width: 28
                    horizontalAlignment: Text.AlignRight
                    text: index === 0 ? "0" : index + "k"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: "DejaVu Sans Mono"
                }
            }
        }

        Repeater {
            model: root.events
            Item {
                readonly property real dAhead: {
                    const L = root.backend.routeLengthM
                    return ((modelData.absM - root.backend.routePositionM) % L + L) % L
                }
                visible: dAhead <= root.windowM
                y: root.yFor(dAhead)

                Rectangle {
                    x: 4; y: -1
                    width: strip.width - 8; height: 2
                    color: modelData.kind === "station" ? Theme.accent : Theme.yellow
                    opacity: modelData.kind === "station" ? 1.0 : 0.75
                }
                Rectangle {
                    visible: modelData.kind === "limit"
                    x: strip.width - 40; y: -22
                    width: 36; height: 20; radius: 10
                    color: Theme.panelInset
                    border.color: Theme.yellow
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: Theme.yellow
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: "DejaVu Sans Mono"
                        font.bold: true
                    }
                }
                Text {
                    visible: modelData.kind === "station"
                    x: 6; y: -18
                    text: "⏹ " + modelData.label
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    elide: Text.ElideRight
                    width: strip.width - 12
                }
            }
        }

        Rectangle { // supervised braking target
            visible: root.backend.distanceToTargetM >= 0
                     && root.backend.distanceToTargetM <= root.windowM
            x: -6
            y: root.yFor(root.backend.distanceToTargetM) - 4
            width: 8; height: 8; radius: 4
            color: Theme.arcTarget
        }

        Text { // train position marker
            anchors.horizontalCenter: parent.horizontalCenter
            y: strip.height - 16
            text: "▲"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBase
        }
    }

    InfoTile {
        id: gradTile
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        implicitHeight: 52
        title: "GRADIENT"
        value: (root.backend.gradientPermille > 0 ? "▲ +" : root.backend.gradientPermille < 0 ? "▼ " : "")
               + root.backend.gradientPermille.toFixed(0)
        unit: "mm/m"
        valueColor: root.backend.gradientPermille > 0 ? Theme.gradientCut : Theme.ok
    }
}

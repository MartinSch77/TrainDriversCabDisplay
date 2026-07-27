import RailDeck
import QtQuick
import QtQuick3D

// Stylised 3D forward view of the line ahead: rails, sleepers and catenary
// masts move with train speed, the scene pitches with the gradient, and the
// supervised braking target appears as a yellow lineside board.
// Scene units: 1 unit = 1 cm.
Rectangle {
    id: root

    property real speedKmh: 0
    property real routePositionM: 0
    property real gradientPermille: 0
    property real distanceToTargetM: -1

    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    border.width: Theme.borderWidth
    clip: true

    View3D {
        anchors.fill: parent
        anchors.margins: 2

        environment: SceneEnvironment {
            clearColor: Theme.panelInset
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 260, 560)
            eulerRotation.x: -14
        }

        DirectionalLight {
            eulerRotation.x: -35
            eulerRotation.y: -25
            brightness: 1.2
        }
        DirectionalLight {
            eulerRotation.x: -10
            eulerRotation.y: 155
            brightness: 0.4
        }

        Node {
            id: track
            eulerRotation.x: root.gradientPermille * 0.5

            Model { // ballast bed
                source: "#Cube"
                scale: Qt.vector3d(30, 0.02, 44)
                position: Qt.vector3d(0, -3, -1900)
                materials: PrincipledMaterial { baseColor: "#10141E"; roughness: 1.0 }
            }

            Model { // left rail
                source: "#Cube"
                scale: Qt.vector3d(0.07, 0.12, 42)
                position: Qt.vector3d(-75, 6, -1900)
                materials: PrincipledMaterial { baseColor: "#8E9AAF"; metalness: 0.7; roughness: 0.35 }
            }
            Model { // right rail
                source: "#Cube"
                scale: Qt.vector3d(0.07, 0.12, 42)
                position: Qt.vector3d(75, 6, -1900)
                materials: PrincipledMaterial { baseColor: "#8E9AAF"; metalness: 0.7; roughness: 0.35 }
            }

            Repeater3D { // sleepers, 60 cm spacing
                model: 40
                Model {
                    source: "#Cube"
                    scale: Qt.vector3d(2.4, 0.05, 0.20)
                    position: Qt.vector3d(0, 1,
                        -index * 60 + (root.routePositionM * 100) % 60)
                    materials: PrincipledMaterial { baseColor: "#39415A"; roughness: 0.9 }
                }
            }

            Repeater3D { // catenary masts, 40 m spacing
                model: 7
                Node {
                    position: Qt.vector3d(-230, 0,
                        -index * 4000 + (root.routePositionM * 100) % 4000 - 400)
                    Model { // pole
                        source: "#Cube"
                        scale: Qt.vector3d(0.14, 5.6, 0.14)
                        position: Qt.vector3d(0, 280, 0)
                        materials: PrincipledMaterial { baseColor: "#4A5570"; roughness: 0.8 }
                    }
                    Model { // cantilever arm
                        source: "#Cube"
                        scale: Qt.vector3d(1.7, 0.08, 0.08)
                        position: Qt.vector3d(85, 520, 0)
                        materials: PrincipledMaterial { baseColor: "#4A5570"; roughness: 0.8 }
                    }
                }
            }

            Node { // braking-target lineside board (compressed 1 m : 1 unit)
                visible: root.distanceToTargetM >= 0 && root.distanceToTargetM < 2200
                position: Qt.vector3d(200, 0, -root.distanceToTargetM)
                Model {
                    source: "#Cube"
                    scale: Qt.vector3d(0.1, 1.6, 0.1)
                    position: Qt.vector3d(0, 80, 0)
                    materials: PrincipledMaterial { baseColor: "#4A5570"; roughness: 0.8 }
                }
                Model {
                    source: "#Cylinder"
                    scale: Qt.vector3d(0.85, 0.08, 0.85)
                    position: Qt.vector3d(0, 190, 0)
                    eulerRotation.x: 90
                    materials: PrincipledMaterial {
                        baseColor: Theme.yellow
                        emissiveFactor: Qt.vector3d(0.6, 0.5, 0.05)
                    }
                }
            }
        }
    }

    Text {
        anchors.top: parent.top
        anchors.topMargin: 7
        anchors.horizontalCenter: parent.horizontalCenter
        text: "LINE AHEAD"
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1
    }
}

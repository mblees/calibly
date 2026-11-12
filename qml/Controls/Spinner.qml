import QtQuick
import QtQuick.Shapes
import Calibly.Theme

// Small rotating arc that shows that something is running. Purely decorative.
//
// Properties:
//   size     edge length in pixels
//   color    color of the arc
//   running  turns while true (and visible)
Item {
    id: root

    property real size: Theme.iconSize
    property color color: Theme.accent
    property bool running: true

    readonly property real radius: (root.size - Theme.focusWidth) / 2

    implicitWidth: root.size
    implicitHeight: root.size

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        // Faint full circle as the track.
        ShapePath {
            strokeColor: Theme.border
            strokeWidth: Theme.focusWidth
            fillColor: "transparent"

            PathAngleArc {
                centerX: root.size / 2
                centerY: root.size / 2
                radiusX: root.radius
                radiusY: root.radius
                startAngle: 0
                sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: root.color
            strokeWidth: Theme.focusWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.size / 2
                centerY: root.size / 2
                radiusX: root.radius
                radiusY: root.radius
                startAngle: -90
                sweepAngle: 100
            }
        }

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: Theme.spinnerDuration
            loops: Animation.Infinite
            running: root.running && root.visible
        }
    }
}

import QtQuick
import QtQuick.Shapes

// Line icon drawn from SVG path data (see Icons.qml), so neither image files
// nor the Qt SVG module are needed. Purely decorative.
//
// Properties:
//   name         icon name, a key of Icons.paths (e.g. "book", "headphones")
//   size         edge length in pixels
//   color        stroke color (fill color for filled icons such as "play")
//   strokeWidth  line width relative to the 24 × 24 grid
Item {
    id: root

    property string name
    property real size: Theme.iconSize
    property color color: Theme.textPrimary
    property real strokeWidth: 2

    readonly property bool filled: Icons.filled.indexOf(name) >= 0

    implicitWidth: size
    implicitHeight: size

    Shape {
        width: 24
        height: 24
        scale: root.size / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: root.filled ? "transparent" : root.color
            strokeWidth: root.strokeWidth
            fillColor: root.filled ? root.color : "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            PathSvg {
                path: Icons.paths[root.name] ?? ""
            }
        }
    }
}

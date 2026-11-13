import QtQuick
import QtQuick.Shapes
import Calibly.Theme

// Color picker: a wheel for the hue (angle) and the saturation (distance
// from the middle), and a slider for the brightness below it. Drag or click
// in the wheel; with the keyboard ←/→ turn the hue, ↑/↓ change the
// saturation.
//
// Properties:
//   color              the chosen color; setting it moves the handle
//   minimumBrightness  the darkest brightness (HSV value) the slider offers
//   text               description for accessibility
// Signals:
//   moved(color color)  the user changed the color
Column {
    id: root

    property color color: Theme.accent
    property real minimumBrightness: 0
    property string text

    // The color as hue, saturation and brightness. Kept here, so the hue
    // survives while the saturation is 0 (a grey has no hue).
    property real hue: 0
    property real saturation: 1
    property real brightness: 1

    signal moved(color color)

    function chosenColor(): color {
        return Qt.hsva(root.hue, root.saturation, root.brightness, 1)
    }

    function choose(hue: real, saturation: real, brightness: real) {
        root.hue = (hue % 1 + 1) % 1
        root.saturation = Math.max(0, Math.min(1, saturation))
        root.brightness = Math.max(root.minimumBrightness, Math.min(1, brightness))
        root.moved(root.chosenColor())
    }

    // Takes over color unless it is the one shown already (it comes back
    // rounded from whoever stores it).
    function takeColor() {
        if (root.color.toString() === root.chosenColor().toString())
            return
        if (root.color.hsvHue >= 0)
            root.hue = root.color.hsvHue
        root.saturation = root.color.hsvSaturation
        root.brightness = root.color.hsvValue
    }

    // Hue and saturation at a point of the picker.
    function pick(x: real, y: real) {
        const radius = picker.width / 2
        const dx = x - radius
        const dy = radius - y
        const angle = Math.atan2(dy, dx) / (2 * Math.PI)
        root.choose(angle, Math.sqrt(dx * dx + dy * dy) / radius, root.brightness)
    }

    onColorChanged: root.takeColor()
    Component.onCompleted: root.takeColor()

    spacing: Theme.spacingM

    Item {
        id: picker

        width: Theme.colorWheelSize
        height: Theme.colorWheelSize
        activeFocusOnTab: true

        Accessible.role: Accessible.ColorChooser
        Accessible.name: root.text
        Accessible.description: root.chosenColor().toString()

        Keys.onLeftPressed: root.choose(root.hue + 1 / 72, root.saturation, root.brightness)
        Keys.onRightPressed: root.choose(root.hue - 1 / 72, root.saturation, root.brightness)
        Keys.onUpPressed: root.choose(root.hue, root.saturation + 0.05, root.brightness)
        Keys.onDownPressed: root.choose(root.hue, root.saturation - 0.05, root.brightness)

        // Hues around the circle, counter-clockwise from the right (red).
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeWidth: -1
                fillGradient: ConicalGradient {
                    centerX: picker.width / 2
                    centerY: picker.height / 2
                    angle: 0

                    GradientStop { position: 0 / 6; color: Qt.hsva(0 / 6, 1, 1, 1) }
                    GradientStop { position: 1 / 6; color: Qt.hsva(1 / 6, 1, 1, 1) }
                    GradientStop { position: 2 / 6; color: Qt.hsva(2 / 6, 1, 1, 1) }
                    GradientStop { position: 3 / 6; color: Qt.hsva(3 / 6, 1, 1, 1) }
                    GradientStop { position: 4 / 6; color: Qt.hsva(4 / 6, 1, 1, 1) }
                    GradientStop { position: 5 / 6; color: Qt.hsva(5 / 6, 1, 1, 1) }
                    GradientStop { position: 6 / 6; color: Qt.hsva(0, 1, 1, 1) }
                }

                PathAngleArc {
                    centerX: picker.width / 2
                    centerY: picker.height / 2
                    radiusX: picker.width / 2
                    radiusY: picker.height / 2
                    startAngle: 0
                    sweepAngle: 360
                }
            }
        }

        // No saturation in the middle.
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeWidth: -1
                fillGradient: RadialGradient {
                    centerX: picker.width / 2
                    centerY: picker.height / 2
                    centerRadius: picker.width / 2
                    focalX: centerX
                    focalY: centerY

                    GradientStop { position: 0; color: Theme.colorWheelCenter }
                    GradientStop { position: 1; color: "transparent" }
                }

                PathAngleArc {
                    centerX: picker.width / 2
                    centerY: picker.height / 2
                    radiusX: picker.width / 2
                    radiusY: picker.height / 2
                    startAngle: 0
                    sweepAngle: 360
                }
            }
        }

        // Lower brightness darkens the whole picker.
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Theme.colorWheelShade
            opacity: 1 - root.brightness
        }

        // Focus ring.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -Theme.focusRingWidth
            radius: width / 2
            color: "transparent"
            visible: picker.activeFocus
            border.width: Theme.focusWidth
            border.color: Theme.textPrimary
        }

        Rectangle {
            readonly property real angle: root.hue * 2 * Math.PI
            readonly property real distance: root.saturation * picker.width / 2

            x: picker.width / 2 + Math.cos(angle) * distance - width / 2
            y: picker.height / 2 - Math.sin(angle) * distance - height / 2
            width: Theme.colorWheelHandleSize
            height: Theme.colorWheelHandleSize
            radius: width / 2
            color: root.chosenColor()
            border.width: Theme.focusWidth
            border.color: Theme.colorWheelHandleBorder
        }

        MouseArea {
            anchors.fill: parent

            onPressed: function(mouse) {
                const radius = picker.width / 2
                const dx = mouse.x - radius
                const dy = mouse.y - radius
                // Only the round wheel, not the corners of its square.
                if (dx * dx + dy * dy > radius * radius) {
                    mouse.accepted = false
                    return
                }
                picker.forceActiveFocus()
                root.pick(mouse.x, mouse.y)
            }
            onPositionChanged: function(mouse) {
                if (pressed)
                    root.pick(mouse.x, mouse.y)
            }
        }
    }

    AppSlider {
        width: Theme.colorWheelSize
        from: root.minimumBrightness
        to: 1
        value: root.brightness
        leftIconName: "moon"

        Accessible.name: qsTr("Helligkeit")

        onMoved: root.choose(root.hue, root.saturation, value)
    }
}

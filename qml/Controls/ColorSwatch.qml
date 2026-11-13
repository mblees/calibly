import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Round color field to click, e.g. a quick choice next to a ColorWheel.
//
// Properties:
//   color    the color it shows
//   checked  it is the chosen color: a ring around it (inherited)
//   text     description for accessibility (inherited, not displayed)
// Signals:
//   clicked() (inherited)
T.AbstractButton {
    id: control

    property color color: Theme.accent

    implicitWidth: Theme.colorSwatchSize
    implicitHeight: Theme.colorSwatchSize
    checkable: false

    Accessible.role: Accessible.Button
    Accessible.name: control.text

    background: Item {
        // Ring for the chosen color and for keyboard focus.
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            visible: control.checked || control.visualFocus
            border.width: Theme.focusWidth
            border.color: Theme.textPrimary
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.focusWidth + Theme.spacingXxs
            radius: width / 2
            color: control.color
            scale: control.down ? 0.9 : 1
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Pill-shaped choice button, e.g. one format in the format selection.
// The chip does not toggle itself: bind "checked" to the current choice and
// change the choice in onClicked. This keeps the selection in one place.
//
// Properties:
//   text     label (inherited)
//   icons    optional list of { name, color }, shown before the label
//   count    number shown after the label, -1 = hidden
//   checked  selected state (inherited), bind it to the current choice
// Signals:
//   clicked() (inherited)
T.AbstractButton {
    id: control

    property var icons: []
    property int count: -1

    readonly property color foregroundColor: control.checked ? Theme.accentForeground : Theme.textSecondary

    implicitWidth: row.implicitWidth + 2 * Theme.spacingM
    implicitHeight: Theme.chipHeight

    font.pixelSize: Theme.fontBody

    Accessible.role: Accessible.RadioButton
    Accessible.name: control.text

    contentItem: Item {
        Row {
            id: row

            anchors.centerIn: parent
            spacing: Theme.spacingS

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingXxs
                visible: control.icons.length > 0

                Repeater {
                    model: control.icons

                    delegate: Icon {
                        required property var modelData

                        name: modelData.name
                        color: control.checked ? Theme.accentForeground : modelData.color
                    }
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                color: control.foregroundColor
                font.pixelSize: control.font.pixelSize
                font.weight: control.checked ? Font.DemiBold : Font.Normal
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.count >= 0
                text: Number(control.count).toLocaleString(Qt.locale(Qt.uiLanguage), "f", 0)
                color: control.checked ? Theme.accentForeground : Theme.textDisabled
                font.pixelSize: Theme.fontSmall
            }
        }
    }

    background: Rectangle {
        radius: height / 2
        color: control.checked ? Theme.accent
                               : control.hovered ? Theme.surfaceRaised : Theme.surface
        border.width: control.visualFocus ? Theme.focusWidth : 0
        border.color: Theme.textPrimary

        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }
}

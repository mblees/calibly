pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Button that shows the current choice and opens a list of options,
// e.g. "Sortieren: Zuletzt geöffnet ▾".
//
// Properties:
//   label         text before the current choice, "" = only the choice
//   iconName      optional icon before the label
//   options       list of option texts
//   currentIndex  index of the current option
// Signals:
//   activated(int index)  the user picked an option
T.Button {
    id: control

    property string label
    property string iconName
    property var options: []
    property int currentIndex: 0

    readonly property string currentText: control.options[control.currentIndex] ?? ""

    signal activated(int index)

    implicitWidth: row.implicitWidth + 2 * Theme.spacingM
    implicitHeight: Theme.chipHeight
    font.pixelSize: Theme.fontBody

    Accessible.role: Accessible.ComboBox
    Accessible.name: control.label !== "" ? control.label + ": " + control.currentText : control.currentText

    onClicked: popup.opened ? popup.close() : popup.open()

    contentItem: Item {
        Row {
            id: row

            anchors.centerIn: parent
            spacing: Theme.spacingS

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName !== ""
                name: control.iconName
                color: Theme.textMuted
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.label !== ""
                text: control.label
                color: Theme.textMuted
                font.pixelSize: control.font.pixelSize
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.currentText
                color: Theme.textPrimary
                font.pixelSize: control.font.pixelSize
            }
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "chevronDown"
                size: Theme.iconSizeSmall + Theme.spacingXxs
                color: Theme.textMuted
            }
        }
    }

    background: Rectangle {
        radius: height / 2
        color: control.hovered || popup.visible ? Theme.surfaceRaised : Theme.surface
        border.width: control.visualFocus ? Theme.focusWidth : 0
        border.color: Theme.textPrimary
    }

    T.Popup {
        id: popup

        y: control.height + Theme.spacingXs
        x: control.width - width
        width: Math.max(control.width, Theme.menuMinWidth)
        implicitHeight: implicitContentHeight + topPadding + bottomPadding
        padding: Theme.spacingXs
        focus: true

        background: Rectangle {
            radius: Theme.radiusM
            color: Theme.surfaceRaised
            border.width: Theme.borderWidth
            border.color: Theme.border
        }

        contentItem: Column {
            spacing: Theme.spacingXxs

            Repeater {
                model: control.options

                delegate: T.ItemDelegate {
                    id: option

                    required property string modelData
                    required property int index

                    width: parent ? parent.width : 0
                    implicitHeight: Theme.controlHeight
                    leftPadding: Theme.spacingS
                    rightPadding: Theme.spacingS
                    text: option.modelData

                    Accessible.name: option.modelData

                    onClicked: {
                        popup.close()
                        control.activated(option.index)
                    }

                    contentItem: Row {
                        spacing: Theme.spacingS

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "check"
                            opacity: option.index === control.currentIndex ? 1 : 0
                            color: Theme.accent
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: option.modelData
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontBody
                        }
                    }

                    background: Rectangle {
                        radius: Theme.radiusS
                        color: option.hovered || option.visualFocus ? Theme.surface : "transparent"
                    }
                }
            }
        }
    }
}

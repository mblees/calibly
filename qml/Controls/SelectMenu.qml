pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme

// Button that shows the current choice and opens a list of options,
// e.g. "Sortieren: Zuletzt geöffnet ▾". Long lists scroll.
//
// Properties:
//   label         text before the current choice, "" = only the choice
//   iconName      optional icon before the label
//   options       list of option texts
//   currentIndex  index of the current option
//   boxed         look like an input field of a form (text left, ▾ right)
//   valueText     shown instead of the current option, "" = the option
//   popupAbove    open the list above the button (e.g. in a bar at the bottom)
// Signals:
//   activated(int index)  the user picked an option
T.Button {
    id: control

    property string label
    property string iconName
    property var options: []
    property int currentIndex: 0
    property bool boxed: false
    property string valueText
    property bool popupAbove: false

    readonly property string currentText: control.valueText !== "" ? control.valueText
                                                                    : control.options[control.currentIndex] ?? ""

    signal activated(int index)

    implicitWidth: row.implicitWidth + 2 * Theme.spacingM
    implicitHeight: control.boxed ? Theme.controlHeight : Theme.chipHeight
    font.pixelSize: Theme.fontBody

    Accessible.role: Accessible.ComboBox
    Accessible.name: control.label !== "" ? control.label + ": " + control.currentText : control.currentText

    onClicked: popup.opened ? popup.close() : popup.open()

    contentItem: Item {
        Row {
            id: row

            // Boxed: aligned left, the chevron sits at the right edge.
            x: control.boxed ? Theme.spacingS + Theme.spacingXxs - control.leftPadding
                             : (parent.width - width) / 2
            anchors.verticalCenter: parent.verticalCenter
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
                visible: !control.boxed
                name: "chevronDown"
                size: Theme.iconSizeSmall + Theme.spacingXxs
                color: Theme.textMuted
            }
        }
        Icon {
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingS + Theme.spacingXxs - control.rightPadding
            anchors.verticalCenter: parent.verticalCenter
            visible: control.boxed
            name: "chevronDown"
            size: Theme.iconSizeSmall + Theme.spacingXxs
            color: Theme.textMuted
        }
    }

    background: Rectangle {
        radius: control.boxed ? Theme.radiusM : height / 2
        color: control.boxed ? Theme.inputBackground
                             : control.hovered || popup.visible ? Theme.surfaceRaised : Theme.surface
        border.width: control.visualFocus ? Theme.focusWidth : control.boxed ? Theme.borderWidth : 0
        border.color: control.visualFocus ? Theme.textPrimary
                                          : control.hovered || popup.visible ? Theme.textMuted : Theme.inputBorder
    }

    T.Popup {
        id: popup

        y: control.popupAbove ? -height - Theme.spacingXs : control.height + Theme.spacingXs
        x: control.boxed ? 0 : control.width - width
        width: Math.max(control.width, Theme.menuMinWidth)
        height: Math.min(implicitContentHeight + topPadding + bottomPadding, Theme.popupMaxHeight)
        padding: Theme.spacingXs
        margins: Theme.spacingS // stays inside the window
        focus: true

        // Show the current option when the list opens.
        onOpened: list.positionViewAtIndex(control.currentIndex, ListView.Contain)

        background: Rectangle {
            radius: Theme.radiusM
            color: Theme.surfaceRaised
            border.width: Theme.borderWidth
            border.color: Theme.border
        }

        contentItem: ListView {
            id: list

            implicitHeight: contentHeight
            clip: true
            spacing: Theme.spacingXxs
            boundsBehavior: Flickable.StopAtBounds
            model: control.options

            ScrollBar.vertical: ScrollBar {}

            delegate: T.ItemDelegate {
                id: option

                required property string modelData
                required property int index

                width: ListView.view ? ListView.view.width : 0
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

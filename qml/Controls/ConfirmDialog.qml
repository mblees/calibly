import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Small dialog that asks before something happens that cannot be undone.
// Opens above everything with open(); Esc, a click outside and "Abbrechen"
// close it without doing anything.
//
// Properties:
//   title        the question, e.g. "3 Titel entfernen?"
//   message      what will happen
//   confirmText  text of the button that goes ahead
//   destructive  the button is red (removing, deleting)
// Signals:
//   accepted()   the user went ahead; the dialog is already closed
T.Popup {
    id: control

    property string title
    property string message
    property string confirmText
    property bool destructive: false

    signal accepted()

    parent: T.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(Theme.confirmDialogWidth, parent ? parent.width - 2 * Theme.spacingXl : Theme.confirmDialogWidth)
    height: implicitContentHeight + topPadding + bottomPadding
    padding: Theme.spacingXl
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    background: Rectangle {
        radius: Theme.radiusL
        color: Theme.dialog
        border.width: Theme.borderWidth
        border.color: Theme.inputBorder
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
    }

    contentItem: Column {
        spacing: Theme.spacingM

        Text {
            width: parent.width
            text: control.title
            wrapMode: Text.WordWrap
            color: Theme.textPrimary
            font.pixelSize: Theme.fontLarge
            font.weight: Font.Bold
        }
        Text {
            width: parent.width
            visible: control.message !== ""
            text: control.message
            wrapMode: Text.WordWrap
            lineHeight: Theme.lineHeightText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
        Item {
            width: parent.width
            height: buttons.height + Theme.spacingS

            Row {
                id: buttons

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: Theme.spacingS

                AppButton {
                    text: qsTr("Abbrechen")
                    onClicked: control.close()
                }
                AppButton {
                    text: control.confirmText
                    variant: control.destructive ? AppButton.Danger : AppButton.Primary
                    onClicked: {
                        control.close()
                        control.accepted()
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme

// Multi-line text input of a form with a fixed height; longer text scrolls.
//
// Properties:
//   text             entered text; bind it to the stored value
//   placeholderText  hint while empty
// Signals:
//   textEdited()     the user changed the text (not emitted for changes of
//                    the bound value)
Item {
    id: root

    property alias text: area.text
    property alias placeholderText: area.placeholderText

    signal textEdited()

    implicitWidth: Theme.searchFieldWidth
    implicitHeight: Theme.textAreaHeight

    Accessible.role: Accessible.EditableText
    Accessible.name: root.placeholderText

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusM
        color: Theme.inputBackground
        border.width: Theme.borderWidth
        border.color: area.activeFocus ? Theme.accent : Theme.inputBorder

        Rectangle {
            anchors.fill: parent
            anchors.margins: -Theme.focusRingWidth
            z: -1
            visible: area.activeFocus
            radius: Theme.radiusM + Theme.focusRingWidth
            color: Qt.alpha(Theme.accent, Theme.focusRingOpacity)
        }
    }

    Flickable {
        id: flick

        anchors.fill: parent
        anchors.margins: Theme.borderWidth
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        // Attached to the Flickable, the text area sizes its content and
        // keeps the cursor visible while typing.
        T.TextArea.flickable: T.TextArea {
            id: area

            leftPadding: Theme.spacingS + Theme.spacingXxs
            rightPadding: Theme.spacingL
            topPadding: Theme.spacingS
            bottomPadding: Theme.spacingS
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            color: Theme.textPrimary
            placeholderTextColor: Theme.textDisabled
            selectionColor: Theme.accent
            selectedTextColor: Theme.accentForeground
            font.pixelSize: Theme.fontBody

            // TextEdit has no textEdited signal: changes while focused are the user's.
            onTextChanged: {
                if (area.activeFocus)
                    root.textEdited()
            }

            Text {
                x: area.leftPadding
                y: area.topPadding
                width: area.width - area.leftPadding - area.rightPadding
                visible: area.text === "" && area.preeditText === ""
                text: area.placeholderText
                wrapMode: Text.WordWrap
                color: area.placeholderTextColor
                font: area.font
            }
        }
    }
}

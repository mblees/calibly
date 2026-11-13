// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Input for a list of short texts (e.g. tags), shown as removable chips.
// Enter or "," adds the typed text, Backspace in the empty input removes the
// last chip. The field does not change its list itself: bind "tags" to the
// stored list and store the list from tagsEdited().
//
// Properties:
//   tags             list of texts
//   placeholderText  hint in the input, e.g. "+ hinzufügen"
// Signals:
//   tagsEdited(list<string> tags)  the user added or removed a text
Rectangle {
    id: root

    property list<string> tags
    property string placeholderText

    signal tagsEdited(list<string> tags)

    function addTyped() {
        const text = input.text.trim()
        input.clear()
        if (text !== "")
            root.tagsEdited(root.tags.concat([text]))
    }

    function removeAt(index: int) {
        const copy = root.tags.slice()
        copy.splice(index, 1)
        root.tagsEdited(copy)
    }

    implicitWidth: Theme.searchFieldWidth
    implicitHeight: Math.max(Theme.controlHeight, flow.implicitHeight + 2 * Theme.spacingXs + 2)
    radius: Theme.radiusM
    color: Theme.inputBackground
    border.width: Theme.borderWidth
    border.color: input.activeFocus ? Theme.accent : Theme.inputBorder

    Accessible.role: Accessible.List

    Rectangle {
        anchors.fill: parent
        anchors.margins: -Theme.focusRingWidth
        z: -1
        visible: input.activeFocus
        radius: Theme.radiusM + Theme.focusRingWidth
        color: Qt.alpha(Theme.accent, Theme.focusRingOpacity)
    }

    // A click on the free space starts typing.
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        onClicked: input.forceActiveFocus()
    }

    Flow {
        id: flow

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spacingS
        anchors.rightMargin: Theme.spacingS
        spacing: Theme.spacingXs

        Repeater {
            model: root.tags

            delegate: Rectangle {
                id: chip

                required property string modelData
                required property int index

                width: chipRow.implicitWidth + Theme.spacingS + Theme.spacingXs
                height: Theme.tagHeight
                radius: height / 2
                color: Theme.surfaceRaised

                Row {
                    id: chipRow

                    x: Theme.spacingS
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.spacingXxs

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: chip.modelData
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSmall
                    }

                    T.AbstractButton {
                        id: removeButton

                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.tagHeight - Theme.spacingXs
                        height: width
                        text: qsTr("%1 entfernen").arg(chip.modelData)

                        Accessible.name: removeButton.text

                        onClicked: root.removeAt(chip.index)

                        contentItem: Item {
                            Icon {
                                anchors.centerIn: parent
                                name: "x"
                                size: Theme.iconSizeSmall
                                color: removeButton.hovered ? Theme.textPrimary : Theme.textMuted
                            }
                        }
                    }
                }
            }
        }

        T.TextField {
            id: input

            width: Math.max(Theme.menuMinWidth / 2, placeholder.implicitWidth + Theme.spacingS)
            height: Theme.tagHeight
            padding: 0
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true
            color: Theme.textPrimary
            selectionColor: Theme.accent
            selectedTextColor: Theme.accentForeground
            font.pixelSize: Theme.fontSmall

            Accessible.role: Accessible.EditableText
            Accessible.name: root.placeholderText

            onAccepted: root.addTyped()
            onTextEdited: {
                if (input.text.endsWith(","))
                    root.addTyped()
            }
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Backspace && input.text === "" && root.tags.length > 0) {
                    root.removeAt(root.tags.length - 1)
                    event.accepted = true
                }
            }
            // Text typed but not confirmed is not lost when leaving the field.
            onActiveFocusChanged: {
                if (!input.activeFocus)
                    root.addTyped()
            }

            Text {
                id: placeholder

                anchors.verticalCenter: parent.verticalCenter
                visible: input.text === "" && input.preeditText === ""
                text: root.placeholderText
                color: Theme.textMuted
                font: input.font
            }
        }
    }
}

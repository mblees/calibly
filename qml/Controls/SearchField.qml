// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Single-line search input with a search icon, a clear button and an
// optional keyboard shortcut hint. Esc clears the text.
//
// Properties:
//   text             entered text (inherited)
//   placeholderText  hint while empty (inherited)
//   shortcutHint     e.g. "Strg K", shown on the right while empty, "" = hidden
// Signals:
//   textEdited() (inherited)
T.TextField {
    id: control

    property string shortcutHint

    implicitWidth: Theme.searchFieldWidth
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.spacingM + Theme.iconSize + Theme.spacingS
    rightPadding: Theme.spacingM + Math.max(clearButton.visible ? clearButton.width : 0,
                                            hint.visible ? hint.width : 0)
    verticalAlignment: TextInput.AlignVCenter

    color: Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accent
    selectedTextColor: Theme.accentForeground
    font.pixelSize: Theme.fontBody

    Accessible.role: Accessible.EditableText
    Accessible.name: control.placeholderText

    Keys.onEscapePressed: function(event) {
        if (control.text !== "") {
            control.clear()
            control.textEdited()
            event.accepted = true
        } else {
            event.accepted = false
        }
    }

    background: Rectangle {
        radius: Theme.radiusM
        color: Theme.surface
        border.width: control.activeFocus ? Theme.borderWidth : 0
        border.color: Theme.textMuted

        Icon {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingM
            anchors.verticalCenter: parent.verticalCenter
            name: "search"
            color: Theme.textMuted
        }

        // QtQuick.Templates does not draw the placeholder itself.
        Text {
            x: control.leftPadding
            width: parent.width - control.leftPadding - control.rightPadding
            anchors.verticalCenter: parent.verticalCenter
            visible: control.text === "" && control.preeditText === ""
            text: control.placeholderText
            elide: Text.ElideRight
            color: control.placeholderTextColor
            font: control.font
        }

        Text {
            id: hint

            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingM
            anchors.verticalCenter: parent.verticalCenter
            visible: control.shortcutHint !== "" && control.text === ""
            text: control.shortcutHint
            color: Theme.textDisabled
            font.pixelSize: Theme.fontSmall

            Rectangle {
                anchors.fill: parent
                anchors.margins: -Theme.spacingXxs
                anchors.leftMargin: -Theme.spacingXs
                anchors.rightMargin: -Theme.spacingXs
                color: "transparent"
                radius: Theme.radiusS
                border.width: Theme.borderWidth
                border.color: Theme.border
            }
        }
    }

    IconButton {
        id: clearButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingXs
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.controlHeight - 2 * Theme.spacingXs
        height: width
        visible: control.text !== ""
        iconName: "x"
        iconSize: Theme.iconSizeSmall + Theme.spacingXxs
        text: qsTr("Suche leeren")

        onClicked: {
            control.clear()
            control.textEdited()
            control.forceActiveFocus()
        }
    }
}

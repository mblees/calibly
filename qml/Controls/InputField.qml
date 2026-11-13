// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Single-line text input of a form. The focused field gets an accent frame.
// Read-only fields show their value muted and without a frame.
//
// Properties:
//   text             entered text (inherited); bind it to the stored value
//   placeholderText  hint while empty (inherited)
//   readOnly         display only (inherited)
//   validator        e.g. IntValidator (inherited)
// Signals:
//   textEdited()     the user changed the text (inherited)
T.TextField {
    id: control

    implicitWidth: Theme.searchFieldWidth
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.spacingS + Theme.spacingXxs
    rightPadding: Theme.spacingS + Theme.spacingXxs
    verticalAlignment: TextInput.AlignVCenter
    selectByMouse: true

    color: control.readOnly ? Theme.textMuted : Theme.textPrimary
    placeholderTextColor: Theme.textDisabled
    selectionColor: Theme.accent
    selectedTextColor: Theme.accentForeground
    font.pixelSize: Theme.fontBody

    Accessible.role: Accessible.EditableText
    Accessible.name: control.placeholderText

    background: Rectangle {
        radius: Theme.radiusM
        color: control.readOnly ? "transparent" : Theme.inputBackground
        border.width: Theme.borderWidth
        border.color: control.activeFocus && !control.readOnly ? Theme.accent : Theme.inputBorder

        // Soft accent glow around the focused field.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -Theme.focusRingWidth
            z: -1
            visible: control.activeFocus && !control.readOnly
            radius: Theme.radiusM + Theme.focusRingWidth
            color: Qt.alpha(Theme.accent, Theme.focusRingOpacity)
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
    }
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Button that shows only an icon.
//
// Properties:
//   iconName   icon, a key of Icons.paths
//   text       description for accessibility (inherited, not displayed)
//   iconSize   icon edge length in pixels
//   iconColor  icon color when not hovered
//   round      round (default) or rounded-square background
//   checked    shown as switched on, e.g. while its panel is open (inherited)
// Signals:
//   clicked() (inherited)
T.AbstractButton {
    id: control

    property string iconName
    property real iconSize: Theme.iconSize
    property color iconColor: Theme.textSecondary
    property bool round: true

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight
    padding: 0

    Accessible.name: control.text

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            color: control.hovered || control.checked ? Theme.textPrimary : control.iconColor
        }
    }

    background: Rectangle {
        radius: control.round ? width / 2 : Theme.radiusM
        color: control.down ? Theme.surfaceRaised
                            : control.checked ? Theme.surfaceSelected
                                              : control.hovered ? Theme.surface : "transparent"
        border.width: control.visualFocus ? Theme.focusWidth : 0
        border.color: Theme.textPrimary

        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }
}

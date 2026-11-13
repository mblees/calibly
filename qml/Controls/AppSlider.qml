// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Horizontal slider with an optional icon on each side,
// e.g. small and large grid icons for the cover size.
//
// Properties:
//   from, to, value, stepSize  range and value (inherited)
//   leftIconName, rightIconName  optional icons, keys of Icons.paths
//   trackColor   color of the track, e.g. on a raised panel
// Signals:
//   moved() (inherited)
T.Slider {
    id: control

    property string leftIconName
    property string rightIconName
    property color trackColor: Theme.surfaceRaised

    readonly property int iconSpace: Theme.iconSize + Theme.spacingS

    implicitWidth: Theme.sliderWidth + (leftIconName !== "" ? iconSpace : 0) + (rightIconName !== "" ? iconSpace : 0)
    implicitHeight: Theme.controlHeight
    leftPadding: control.leftIconName !== "" ? control.iconSpace : 0
    rightPadding: control.rightIconName !== "" ? control.iconSpace : 0

    Accessible.role: Accessible.Slider

    Icon {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        visible: control.leftIconName !== ""
        name: control.leftIconName
        size: Theme.iconSizeSmall
        color: Theme.textMuted
    }

    Icon {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        visible: control.rightIconName !== ""
        name: control.rightIconName
        color: Theme.textMuted
    }

    background: Rectangle {
        x: control.leftPadding
        y: (control.height - height) / 2
        width: control.availableWidth
        height: Theme.progressHeight + Theme.borderWidth
        radius: height / 2
        color: control.trackColor

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: Theme.textMuted
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: (control.height - height) / 2
        width: Theme.sliderHandleSize
        height: Theme.sliderHandleSize
        radius: width / 2
        color: control.pressed ? Theme.textPrimary : Theme.textSecondary
        border.width: control.visualFocus ? Theme.focusWidth : 0
        border.color: Theme.accent
    }
}

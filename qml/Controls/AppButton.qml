// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Text button with an optional leading icon.
//
// Properties:
//   text      label (inherited)
//   iconName  optional icon, a key of Icons.paths
//   variant   AppButton.Primary   – main action, accent color
//             AppButton.Read      – reading action, blue
//             AppButton.Danger    – removes or deletes something, red
//             AppButton.Secondary – everything else (default)
// Signals:
//   clicked() (inherited)
T.Button {
    id: control

    enum Variant {
        Primary,
        Read,
        Secondary,
        Danger
    }

    property string iconName
    property int variant: AppButton.Secondary

    readonly property color baseColor: {
        switch (control.variant) {
        case AppButton.Primary:
            return Theme.accent;
        case AppButton.Read:
            return Theme.read;
        case AppButton.Danger:
            return Theme.danger;
        default:
            return Theme.surfaceRaised;
        }
    }
    readonly property color foregroundColor: {
        switch (control.variant) {
        case AppButton.Primary:
            return Theme.accentForeground;
        case AppButton.Read:
            return Theme.readForeground;
        case AppButton.Danger:
            return Theme.dangerForeground;
        default:
            return Theme.textPrimary;
        }
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: Theme.spacingL
    rightPadding: Theme.spacingL
    topPadding: Theme.spacingS
    bottomPadding: Theme.spacingS
    spacing: Theme.spacingS

    font.pixelSize: Theme.fontBody
    font.weight: Font.DemiBold

    Accessible.name: control.text

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row
            anchors.centerIn: parent
            spacing: control.spacing

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName !== ""
                name: control.iconName
                color: control.foregroundColor
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.foregroundColor
            }
        }
    }

    background: Rectangle {
        implicitHeight: Theme.controlHeight
        radius: Theme.radiusM
        opacity: control.enabled ? 1 : Theme.disabledOpacity
        color: control.down ? Qt.darker(control.baseColor, 1.15)
                            : control.hovered ? Qt.lighter(control.baseColor, 1.1)
                                              : control.baseColor
        border.width: control.visualFocus ? Theme.focusWidth : 0
        border.color: Theme.textPrimary

        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }
    }
}

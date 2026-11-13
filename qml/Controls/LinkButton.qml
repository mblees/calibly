// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Unobtrusive text button for secondary actions: small muted text with an
// optional leading icon, brighter when hovered.
//
// Properties:
//   text       label (inherited)
//   iconName   optional icon, a key of Icons.paths
//   iconColor  icon color when not hovered
// Signals:
//   clicked() (inherited)
T.AbstractButton {
    id: control

    property string iconName
    property color iconColor: Theme.textMuted

    readonly property bool highlighted: control.hovered || control.visualFocus

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    topPadding: Theme.spacingXs
    bottomPadding: Theme.spacingXs
    spacing: Theme.spacingXs

    font.pixelSize: Theme.fontSmall

    Accessible.name: control.text

    contentItem: Row {
        spacing: control.spacing
        opacity: control.enabled ? 1 : Theme.disabledOpacity

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            visible: control.iconName !== ""
            name: control.iconName
            size: Theme.iconSizeSmall + Theme.spacingXxs
            color: control.highlighted ? Theme.textPrimary : control.iconColor
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: control.text
            font.pixelSize: control.font.pixelSize
            font.underline: control.visualFocus
            color: control.highlighted ? Theme.textPrimary : Theme.textMuted
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}

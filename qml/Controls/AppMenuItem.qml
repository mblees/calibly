// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// One entry of an AppMenu: icon and text. An entry that is not visible
// takes no space in the menu.
//
// Properties:
//   text      label (inherited)
//   iconName  icon, a key of Icons.paths; "" keeps the space free, so the
//             texts of all entries line up
//   danger    the entry removes or deletes something: red
// Signals:
//   triggered() (inherited)  the entry was chosen; the menu closes itself
T.MenuItem {
    id: control

    property string iconName
    property bool danger: false

    readonly property color foreground: !control.enabled ? Theme.textDisabled
                                        : control.danger ? Theme.danger : Theme.textPrimary

    // Not from the row: it has no size while the menu is closed, and the
    // menu needs the width of its entries before it opens.
    implicitWidth: iconSpace.width + row.spacing + label.implicitWidth + leftPadding + rightPadding
    implicitHeight: Theme.controlHeight - Theme.spacingXxs
    height: control.visible ? control.implicitHeight : 0
    leftPadding: Theme.spacingM
    rightPadding: Theme.spacingL
    font.pixelSize: Theme.fontBody

    Accessible.name: control.text

    contentItem: Item {
        Row {
            id: row

            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spacingM

            Item {
                id: iconSpace

                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSize - Theme.spacingXxs
                height: width

                Icon {
                    anchors.fill: parent
                    visible: control.iconName !== ""
                    name: control.iconName
                    size: parent.width
                    color: control.danger && control.enabled ? Theme.danger
                           : control.enabled ? Theme.textSecondary : Theme.textDisabled
                }
            }
            Text {
                id: label

                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                color: control.foreground
                font.pixelSize: control.font.pixelSize
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusS
        color: control.enabled && (control.highlighted || control.hovered) ? Theme.hoverTint : "transparent"
    }
}

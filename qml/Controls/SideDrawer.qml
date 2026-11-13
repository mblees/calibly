// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Panel that slides in from the right edge over a dimmed window.
// Closes with Esc or a click outside. Put the content into contentItem.
//
// Properties and functions (inherited from Drawer):
//   open(), close(), opened, contentItem
T.Drawer {
    id: control

    parent: T.Overlay.overlay
    edge: Qt.RightEdge
    width: Math.min(Theme.drawerWidth, parent ? parent.width : Theme.drawerWidth)
    height: parent ? parent.height : 0
    padding: 0
    dragMargin: 0 // no swiping open from the window edge
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

    enter: Transition {
        NumberAnimation { property: "position"; to: 1; duration: Theme.durationDrawer; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "position"; to: 0; duration: Theme.durationDrawer; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        color: Theme.sidebar

        Rectangle {
            width: Theme.borderWidth
            height: parent.height
            color: Theme.border
        }
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
    }
}

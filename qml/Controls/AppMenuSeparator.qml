// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Line between two groups of entries of an AppMenu. A separator that is not
// visible takes no space in the menu.
T.MenuSeparator {
    id: control

    implicitHeight: Theme.borderWidth + 2 * Theme.spacingXs
    height: control.visible ? control.implicitHeight : 0
    topPadding: Theme.spacingXs
    bottomPadding: Theme.spacingXs

    contentItem: Rectangle {
        implicitHeight: Theme.borderWidth
        color: Theme.border
    }
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Theme

// Row of Chips for one choice, e.g. the format selection.
// The chips bind their "checked" state to the current choice:
//
//   ChipGroup {
//       Chip { text: qsTr("Alle"); checked: view.filter === 0; onClicked: view.filter = 0 }
//       Chip { text: qsTr("E-Books"); checked: view.filter === 1; onClicked: view.filter = 1 }
//   }
Row {
    spacing: Theme.spacingS

    Accessible.role: Accessible.Grouping
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// Base type of all main views. Main.qml sets title and iconName from the
// navigation model when the view is loaded.
Item {
    property string title
    property string iconName
}

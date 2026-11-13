// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// The view "Neu" of the sidebar: only the titles added in the last minutes
// (LibraryView.newMinutes), newest first. Editing a title does not make it
// new; it leaves the view on its own when the time is up.
LibraryView {
    newOnly: true
}

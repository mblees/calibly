// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// The view "Serien" of the sidebar: every series as a stack (also one with
// a single volume); a click on a stack shows its volumes in order. It is the
// library view restricted to series, so everything else works as there:
// search, format selection, sorting, selection, detail view.
LibraryView {
    seriesOnly: true
}

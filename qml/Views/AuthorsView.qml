// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick

// The view "Autoren" of the sidebar: every author as a stack (by the first
// author of a title). A click on a stack shows the works of that author,
// their series again as stacks. It is the library view restricted to
// authors, so everything else works as there: search, format selection,
// sorting, selection, detail view.
LibraryView {
    authorsOnly: true
}

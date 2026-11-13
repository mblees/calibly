// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Controls

// Context menu of a card in "Weitermachen" (right click): continue, the
// details, or take it out of the list – which sets its progress back to
// the start. Presentation only: it reports what was chosen.
//
// Properties:
//   kind       "ebook" or "audiobook": the format of the card
//   available  its file was found; otherwise continuing is disabled
// Signals:
//   continueRequested()  "Weiterlesen" / "Weiterhören"
//   openRequested()      "Details"
//   removeRequested()    "Aus Weitermachen entfernen"
AppMenu {
    id: root

    property string kind: "ebook"
    property bool available: true

    signal continueRequested()
    signal openRequested()
    signal removeRequested()

    AppMenuItem {
        enabled: root.available
        iconName: root.kind === "audiobook" ? "headphones" : "book"
        text: root.kind === "audiobook" ? qsTr("Weiterhören") : qsTr("Weiterlesen")
        onTriggered: root.continueRequested()
    }
    AppMenuItem {
        iconName: "list"
        text: qsTr("Details")
        onTriggered: root.openRequested()
    }
    AppMenuSeparator {}
    AppMenuItem {
        iconName: "reset"
        danger: true
        text: qsTr("Aus Weitermachen entfernen")
        onTriggered: root.removeRequested()
    }
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Controls

// Context menu of a title in the gallery (right click on a cover). What it
// offers depends on what was clicked:
//   * a title: details, read / listen, metadata, mark as finished,
//     dissolve a combination, select, remove
//   * one of several selected titles (or a selected series stack): the
//     actions for all of them, as in the BulkActionBar
//   * a series stack: open it, select it, dissolve the series
// Presentation only: it reports what was chosen, the view does it.
//
// Properties:
//   stack              the cover is a series stack
//   authorStack        the stack stands for an author ("Autoren"): it can
//                      be opened, but is no series to dissolve
//   hasEbook, hasAudiobook   the formats the title has
//   readsInApp         the ebook opens in calibly's reader: the menu also
//                      offers "Mit Standardprogramm öffnen"
//   ebookFinished, audiobookFinished   read / listened to the end
//   fileMissing        a file is missing: reading / listening is disabled
//   selected           the title is selected
//   selectionCount     number of selected titles
//   canMerge           "Kombinieren" is possible with the selection
//   canSplit           the title is combined: "Kombination auflösen"
//   busy               actions that change the library are disabled, e.g.
//                      while an import is running
//   exporting          an export is running: exporting is disabled
//   bulk (read-only)   the menu is about the whole selection: the title (or
//                      stack) is selected and not the only one
// Signals:
//   openRequested()    the details of the title, or the series of a stack
//   readRequested(), listenRequested()
//   openExternallyRequested()   the ebook in the default application
//   editRequested()    metadata of the title (disabled for a selection)
//   finishedToggled(string kind)   "ebook" or "audiobook"
//   selectToggled()    select the title / take it out of the selection
//   clearSelectionRequested()   bulk: end the selection
//   mergeRequested()   bulk: "Kombinieren", the selected titles into one entry
//   seriesClearRequested()    bulk: "Aus der Serie nehmen"
//   seriesRemoveRequested()   a stack: "Serie auflösen …"
//   seriesRequested()  bulk: "Als Serie zusammenfassen …"
//   markRequested(bool finished)   bulk: all as finished / as new
//   splitRequested()   "Kombination auflösen"
//   exportRequested()  bulk: copy the files of the selected titles into a folder
//   removeRequested()  remove the title, or the selection (bulk)
AppMenu {
    id: root

    property bool stack: false
    property bool authorStack: false
    property bool hasEbook: false
    property bool hasAudiobook: false
    property bool readsInApp: false
    property bool ebookFinished: false
    property bool audiobookFinished: false
    property bool fileMissing: false
    property bool selected: false
    property int selectionCount: 0
    property bool canMerge: false
    property bool canSplit: false
    property bool busy: false
    property bool exporting: false

    readonly property bool bulk: root.selected && root.selectionCount > 1
    // A series stack that is not part of a selection.
    readonly property bool stackOnly: root.stack && !root.bulk
    // One title, not a stack and not a selection.
    readonly property bool single: !root.stack && !root.bulk
    readonly property bool bothFormats: root.hasEbook && root.hasAudiobook

    signal openRequested()
    signal readRequested()
    signal openExternallyRequested()
    signal listenRequested()
    signal editRequested()
    signal finishedToggled(string kind)
    signal selectToggled()
    signal clearSelectionRequested()
    signal mergeRequested()
    signal seriesRequested()
    signal markRequested(bool finished)
    signal splitRequested()
    signal seriesClearRequested()
    signal seriesRemoveRequested()
    signal exportRequested()
    signal removeRequested()

    // --- a series stack --------------------------------------------------------
    AppMenuItem {
        visible: root.stackOnly
        iconName: root.authorStack ? "user" : "layers"
        text: root.authorStack ? qsTr("Autor öffnen") : qsTr("Serie öffnen")
        onTriggered: root.openRequested()
    }
    AppMenuItem {
        visible: root.stackOnly && !root.authorStack
        enabled: !root.busy
        iconName: "split"
        text: qsTr("Serie auflösen …")
        onTriggered: root.seriesRemoveRequested()
    }

    // --- one title ---------------------------------------------------------------
    AppMenuItem {
        visible: root.single
        iconName: "list"
        text: qsTr("Details")
        onTriggered: root.openRequested()
    }
    AppMenuItem {
        visible: root.single && root.hasEbook
        enabled: !root.fileMissing
        iconName: "book"
        text: qsTr("Lesen")
        onTriggered: root.readRequested()
    }
    AppMenuItem {
        visible: root.single && root.hasEbook && root.readsInApp
        enabled: !root.fileMissing
        iconName: "externalLink"
        text: qsTr("Mit Standardprogramm öffnen")
        onTriggered: root.openExternallyRequested()
    }
    AppMenuItem {
        visible: root.single && root.hasAudiobook
        enabled: !root.fileMissing
        iconName: "headphones"
        text: qsTr("Hören")
        onTriggered: root.listenRequested()
    }
    AppMenuSeparator {
        visible: root.single
    }

    // --- one title or the selection ------------------------------------------------
    // Metadata are edited for one title at a time.
    AppMenuItem {
        visible: !root.stackOnly
        enabled: !root.bulk
        iconName: "edit"
        text: qsTr("Metadaten bearbeiten")
        onTriggered: root.editRequested()
    }
    AppMenuItem {
        visible: root.single && root.hasEbook
        iconName: "check"
        text: root.bothFormats ? (root.ebookFinished ? qsTr("E-Book als nicht fertig markieren")
                                                     : qsTr("E-Book als fertig markieren"))
                               : (root.ebookFinished ? qsTr("Als nicht fertig markieren")
                                                     : qsTr("Als fertig markieren"))
        onTriggered: root.finishedToggled("ebook")
    }
    AppMenuItem {
        visible: root.single && root.hasAudiobook
        iconName: "check"
        text: root.bothFormats ? (root.audiobookFinished ? qsTr("Hörbuch als nicht fertig markieren")
                                                         : qsTr("Hörbuch als fertig markieren"))
                               : (root.audiobookFinished ? qsTr("Als nicht fertig markieren")
                                                         : qsTr("Als fertig markieren"))
        onTriggered: root.finishedToggled("audiobook")
    }
    AppMenuItem {
        visible: root.bulk
        iconName: "check"
        text: qsTr("Alle als fertig markieren")
        enabled: !root.busy
        onTriggered: root.markRequested(true)
    }
    AppMenuItem {
        visible: root.bulk
        iconName: "reset"
        text: qsTr("Alle als neu markieren …")
        enabled: !root.busy
        onTriggered: root.markRequested(false)
    }
    AppMenuItem {
        visible: root.bulk
        enabled: root.canMerge && !root.busy
        iconName: "link"
        text: qsTr("Kombinieren")
        onTriggered: root.mergeRequested()
    }
    AppMenuItem {
        visible: root.bulk
        enabled: !root.busy
        iconName: "layers"
        text: qsTr("Als Serie zusammenfassen …")
        onTriggered: root.seriesRequested()
    }
    AppMenuItem {
        visible: root.bulk
        enabled: !root.busy
        iconName: "x"
        text: qsTr("Aus der Serie nehmen")
        onTriggered: root.seriesClearRequested()
    }
    AppMenuItem {
        visible: root.single && root.canSplit
        enabled: !root.busy
        iconName: "split"
        text: qsTr("Kombination auflösen")
        onTriggered: root.splitRequested()
    }
    AppMenuItem {
        visible: root.bulk
        enabled: !root.exporting
        iconName: "upload"
        text: qsTr("%Ln Titel exportieren …", "", root.selectionCount)
        onTriggered: root.exportRequested()
    }
    AppMenuItem {
        visible: root.single || root.stackOnly
        iconName: root.selected ? "x" : "plus"
        text: root.selected ? qsTr("Auswahl aufheben") : qsTr("Auswählen")
        onTriggered: root.selectToggled()
    }
    AppMenuItem {
        visible: root.bulk
        iconName: "x"
        text: qsTr("Auswahl aufheben")
        onTriggered: root.clearSelectionRequested()
    }
    AppMenuSeparator {
        visible: !root.stackOnly
    }
    AppMenuItem {
        visible: !root.stackOnly
        enabled: !root.busy
        iconName: "trash"
        danger: true
        text: root.bulk ? qsTr("%Ln Titel entfernen", "", root.selectionCount) : qsTr("Entfernen")
        onTriggered: root.removeRequested()
    }
}

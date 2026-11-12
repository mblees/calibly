import QtQuick
import Calibly.Controls

// Context menu of a title in the gallery (right click on a cover). What it
// offers depends on what was clicked:
//   * a title: details, read / listen, metadata, mark as finished, select,
//     remove
//   * one of several selected titles: the actions for all of them, as in
//     the BulkActionBar
//   * a series stack: open the series
// Presentation only: it reports what was chosen, the view does it.
//
// Properties:
//   stack              the cover is a series stack
//   hasEbook, hasAudiobook   the formats the title has
//   ebookFinished, audiobookFinished   read / listened to the end
//   fileMissing        a file is missing: reading / listening is disabled
//   selected           the title is selected
//   selectionCount     number of selected titles
//   canMerge           "Zu einem Werk verbinden" is possible with the selection
//   busy               actions that change the library are disabled, e.g.
//                      while an import is running
//   bulk (read-only)   the menu is about the whole selection: the title is
//                      selected and not the only one
// Signals:
//   openRequested()    the details of the title, or the series of a stack
//   readRequested(), listenRequested()
//   editRequested()    metadata of the title, or of the selection (bulk)
//   finishedToggled(string kind)   "ebook" or "audiobook"
//   selectToggled()    select the title / take it out of the selection
//   clearSelectionRequested()   bulk: end the selection
//   mergeRequested()   bulk: join the selected titles into one work
//   removeRequested()  remove the title, or the selection (bulk)
AppMenu {
    id: root

    property bool stack: false
    property bool hasEbook: false
    property bool hasAudiobook: false
    property bool ebookFinished: false
    property bool audiobookFinished: false
    property bool fileMissing: false
    property bool selected: false
    property int selectionCount: 0
    property bool canMerge: false
    property bool busy: false

    readonly property bool bulk: !root.stack && root.selected && root.selectionCount > 1
    // One title, not a stack and not a selection.
    readonly property bool single: !root.stack && !root.bulk
    readonly property bool bothFormats: root.hasEbook && root.hasAudiobook

    signal openRequested()
    signal readRequested()
    signal listenRequested()
    signal editRequested()
    signal finishedToggled(string kind)
    signal selectToggled()
    signal clearSelectionRequested()
    signal mergeRequested()
    signal removeRequested()

    // --- a series stack --------------------------------------------------------
    AppMenuItem {
        visible: root.stack
        iconName: "layers"
        text: qsTr("Serie öffnen")
        onTriggered: root.openRequested()
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
    AppMenuItem {
        visible: !root.stack
        iconName: "edit"
        text: root.bulk ? qsTr("Metadaten der %Ln Titel bearbeiten", "", root.selectionCount)
                        : qsTr("Metadaten bearbeiten")
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
        enabled: root.canMerge && !root.busy
        iconName: "link"
        text: qsTr("Zu einem Werk verbinden")
        onTriggered: root.mergeRequested()
    }
    AppMenuItem {
        visible: root.single
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
        visible: !root.stack
    }
    AppMenuItem {
        visible: !root.stack
        enabled: !root.busy
        iconName: "trash"
        danger: true
        text: root.bulk ? qsTr("%Ln Titel entfernen", "", root.selectionCount) : qsTr("Entfernen")
        onTriggered: root.removeRequested()
    }
}

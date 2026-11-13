// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// View A of the mockup: all works as a cover grid with search, format
// selection, sorting and cover size. Filtering and sorting happen in C++
// (WorkFilterModel); this view only holds the chosen values.
// Above the grid, and scrolling with it: the row "Weitermachen" with the
// titles that were opened last and are not finished (ContinueModel).
// View D: files dropped on free space become new works, files dropped on a
// cover become formats of that work (ImportController); the import queue
// at the bottom right shows how it goes.
// Series: the volumes of a series are one stack ("Gruppieren: Serien"); a
// click on it shows its volumes, "‹" leads back.
// Selection: the circle on a cover, Ctrl + click or Ctrl + A select works;
// the bar at the bottom then edits their metadata, joins them into one work
// or removes them; the Delete key removes them too (after asking, as the
// bar does). Esc ends the selection, then leaves an opened series.
// A right click on a cover opens a menu with the actions for that title, or
// for the selection if the title is one of several selected ones.
//
// Properties:
//   seriesOnly   show only the series, each as a stack: the view "Serien" of
//                the sidebar (see SeriesView)
//   authorsOnly  the view "Autoren" (AuthorsView), in one of two layouts
//                (instead of sorting, authors are always by last name):
//                every author as a stack – an opened author shows the works
//                with their series as stacks, an opened series of it its
//                volumes – or every author as a heading with all its works
//                below, one by one ("shelves").
//   newOnly      only works added in the last newMinutes minutes, newest
//                first: the view "Neu" (NewView)
ViewBase {
    id: root

    property bool seriesOnly: false
    property bool authorsOnly: false
    property bool newOnly: false
    readonly property int newMinutes: 10
    // The series whose volumes are shown, "" = all works: an opened stack.
    property string openedSeries: ""
    // The author whose works are shown (view "Autoren"), "" = all authors.
    property string openedAuthor: ""
    // The works the question of mergeDialog, removeDialog, seriesDialog or
    // markNewDialog is about: the
    // selection (bar, menu of a selected title) or the title of the menu.
    property list<string> actionIds: []
    property string actionTitle
    property bool actionOnSelection: false
    // What removeDialog removes: "ebook" / "audiobook" = only that format
    // (the tab "E-Books" / "Hörbücher"), "" = the whole titles.
    property string actionKind

    readonly property int total: LibraryController.works.count
    readonly property bool filtered: works.workCount !== root.total
    // "Weitermachen" shows at most this many titles, and none while searching.
    readonly property int continueLimit: 12
    readonly property bool showContinue: continueWorks.count > 0 && search.text === ""
                                         && !root.seriesOnly && !root.authorsOnly && !root.newOnly
                                         && root.openedSeries === ""
    // The list of all authors: author stacks, or shelves.
    readonly property bool authorList: root.authorsOnly && root.openedAuthor === ""
    readonly property bool authorShelves: root.authorList && settings.authorLayout === 1
    // The view that shows the works: the cover grid, or the shelves of the authors.
    readonly property Flickable activeView: root.authorShelves ? shelves : grid
    // The list of all series (or author stacks) has only stacks, which cannot
    // be selected; the works on the shelves can.
    readonly property bool canSelect: works.count > 0 && !(root.seriesOnly && root.openedSeries === "")
                                      && !(root.authorList && !root.authorShelves)
    readonly property bool popupOpen: detail.visible || metadataEditor.visible
                                      || mergeDialog.visible || removeDialog.visible || workMenu.visible
                                      || seriesOrderDialog.visible
    // The view "Serien" without any series (and without a filter that hides them).
    readonly property bool noSeries: root.seriesOnly && root.openedSeries === "" && search.text === ""
                                     && settings.formatFilter === WorkFilterModel.AllFormats
    // The same for the views "Autoren" and "Neu".
    readonly property bool noAuthors: root.authorList && search.text === ""
                                      && settings.formatFilter === WorkFilterModel.AllFormats
    readonly property bool noNew: root.newOnly && search.text === ""
                                  && settings.formatFilter === WorkFilterModel.AllFormats

    // Back to the top of the grid, including its header.
    function scrollToTop() {
        grid.contentY = grid.originY - grid.topMargin
        shelves.contentY = shelves.originY - shelves.topMargin
    }

    // The context menu of a cover. info: the work (or stack) as the
    // delegates know it: workId, title, series (or the name of the stack),
    // coverUrl, coverIsSquare, stack, authorStack, hasEbook, hasAudiobook,
    // readProgress, listenProgress, workState, selected, canSplit.
    function openWorkMenu(item: Item, x: real, y: real, info: var) {
        workMenu.workId = info.workId
        workMenu.title = info.title
        workMenu.series = info.series
        workMenu.coverUrl = info.coverUrl
        workMenu.coverIsSquare = info.coverIsSquare
        workMenu.stack = info.stack
        workMenu.authorStack = info.authorStack
        workMenu.hasEbook = info.hasEbook
        workMenu.readsInApp = info.hasEbook && ReaderController.canRead(info.workId)
        workMenu.hasAudiobook = info.hasAudiobook
        workMenu.ebookFinished = info.readProgress >= 1
        workMenu.audiobookFinished = info.listenProgress >= 1
        workMenu.fileMissing = info.workState === WorkState.FileMissing
        workMenu.selected = info.selected
        workMenu.canSplit = info.canSplit
        workMenu.popup(item, x, y)
    }

    // Continues reading / listening where the user stopped: audiobooks play
    // in calibly, EPUB, PDF and text books open in the reader, other ebooks
    // in the default application. A missing file shows as "DATEI FEHLT" on
    // the cover.
    function continueWith(workId: string, kind: string) {
        if (kind === "audiobook")
            PlayerController.play(workId)
        else
            ReaderController.read(workId)
    }

    function resetFilters() {
        search.clear()
        settings.formatFilter = WorkFilterModel.AllFormats
    }

    function openSeries(series: string) {
        works.clearSelection()
        root.openedSeries = series
    }

    function closeSeries() {
        works.clearSelection()
        root.openedSeries = ""
    }

    function openAuthor(author: string) {
        works.clearSelection()
        root.openedSeries = ""
        root.openedAuthor = author
    }

    function closeAuthor() {
        works.clearSelection()
        root.openedSeries = ""
        root.openedAuthor = ""
    }

    // "‹" and Esc: out of the opened series, then out of the opened author.
    function goBack() {
        if (root.openedSeries !== "")
            root.closeSeries()
        else
            root.closeAuthor()
    }

    // "Metadaten" of the bar: possible with one selected work only.
    function editSelection() {
        const ids = works.selectedIds
        if (ids.length === 1)
            metadataEditor.openFor(ids[0])
    }

    // "Als Serie …": asks for the name, filled with a series the selected
    // works have.
    function askSeries() {
        root.actionIds = works.selectedIds
        seriesDialog.count = root.actionIds.length
        seriesDialog.name = LibraryController.suggestedSeries(root.actionIds)
        seriesDialog.open()
    }

    // "Alle als neu markieren" clears positions: asked first.
    function askMarkNew() {
        root.actionIds = works.selectedIds
        markNewDialog.open()
    }

    // Asks before the selected works are combined into one.
    function askMerge() {
        root.actionIds = works.selectedIds
        root.actionOnSelection = true
        root.actionTitle = LibraryController.mergeTitle(root.actionIds)
        mergeDialog.open()
    }

    // Asks before works are removed: the selection, or one title (with its name).
    // In the tabs "E-Books" and "Hörbücher" only that format is removed.
    function askRemove(workIds: list<string>, title: string) {
        root.actionIds = workIds
        root.actionOnSelection = title === ""
        root.actionTitle = title
        root.actionKind = settings.formatFilter === WorkFilterModel.Ebook ? "ebook"
                        : settings.formatFilter === WorkFilterModel.Audiobook ? "audiobook" : ""
        removeDialog.open()
    }

    // "Exportieren …": asks where the export folder goes; ExportController
    // copies the files, ExportHint (Main.qml) shows how far it is.
    function askExport() {
        exportDialog.workIds = works.selectedIds
        exportDialog.currentFolder = ExportController.lastFolder
        exportDialog.open()
    }

    // Choices that are kept between starts (stored by Qt in the user settings).
    Settings {
        id: settings

        category: "library"

        property int formatFilter: WorkFilterModel.AllFormats
        property int sortOrder: WorkFilterModel.LastOpened
        property real tileWidth: Theme.coverTileMinWidth
        property bool groupSeries: true
        // The view "Autoren": 0 = author stacks, 1 = shelves.
        property int authorLayout: 0
    }

    WorkFilterModel {
        id: works

        sourceModel: LibraryController.works
        formatFilter: settings.formatFilter
        sortOrder: root.newOnly ? WorkFilterModel.Added
                   : root.authorList ? WorkFilterModel.Author
                   : root.authorsOnly ? WorkFilterModel.Series : settings.sortOrder
        searchText: search.text
        groupSeries: root.seriesOnly || root.authorsOnly || (!root.newOnly && settings.groupSeries)
        seriesOnly: root.seriesOnly
        seriesFilter: root.openedSeries
        groupAuthors: root.authorsOnly
        authorFilter: root.openedAuthor
        addedWithin: root.newOnly ? root.newMinutes * 60 : 0

        // Deferred: countChanged arrives while the GridView is still applying
        // the row changes; scrolling right away would create duplicate tiles.
        onCountChanged: Qt.callLater(root.scrollToTop)
        // The opened series (or author) is gone (its volumes were removed,
        // merged or renamed): back to where it was opened from.
        onWorkCountChanged: {
            if ((root.openedSeries !== "" || root.openedAuthor !== "") && works.workCount === 0
                    && search.text === "" && settings.formatFilter === WorkFilterModel.AllFormats)
                Qt.callLater(root.goBack)
        }
    }

    // The format selection also applies to "Weitermachen".
    ContinueModel {
        id: continueWorks

        sourceModel: LibraryController.works
        formatFilter: settings.formatFilter
        limit: root.continueLimit
    }

    Shortcut {
        sequences: [StandardKey.SelectAll]
        enabled: root.canSelect && !root.popupOpen
        onActivated: works.selectAll()
    }
    // Entf / Delete: removes the selected titles, after asking (removeDialog;
    // in the tabs "E-Books" / "Hörbücher" only that format). Not while
    // typing in the search field or while an import may still add to them.
    Shortcut {
        sequences: [StandardKey.Delete]
        enabled: works.selectedCount > 0 && !root.popupOpen && !search.activeFocus && !ImportController.busy
        onActivated: root.askRemove(works.selectedIds, "")
    }
    Shortcut {
        sequence: "Esc"
        enabled: !root.popupOpen && !search.activeFocus
                 && (works.selectedCount > 0 || root.openedSeries !== "" || root.openedAuthor !== "")
        onActivated: {
            if (works.selectedCount > 0)
                works.clearSelection()
            else
                root.goBack()
        }
    }

    Shortcut {
        sequences: ["Ctrl+K", StandardKey.Find]
        onActivated: {
            search.forceActiveFocus()
            search.selectAll()
        }
    }

    // --- toolbar: search, add and cover size ---------------------------------
    SearchField {
        id: search

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: Theme.spacingXl
        anchors.topMargin: Theme.spacingL
        width: Math.min(Theme.searchFieldWidth,
                        root.width - sizeSlider.width - addButton.width - 4 * Theme.spacingXl)
        placeholderText: qsTr("Titel, Autor, Sprecher, Serie …")
        shortcutHint: qsTr("Strg K")
    }

    AppButton {
        id: addButton

        anchors.left: search.right
        anchors.leftMargin: Theme.spacingM
        anchors.verticalCenter: search.verticalCenter
        visible: LibraryController.canImport
        text: qsTr("Hinzufügen")
        iconName: "plus"
        variant: AppButton.Primary

        onClicked: importDialog.open()
    }

    FileDialog {
        id: importDialog

        title: qsTr("E-Books und Hörbücher hinzufügen")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("E-Books und Hörbücher (%1)").arg(LibraryController.fileNameFilters.join(" ")),
                      qsTr("Alle Dateien (*)")]

        onAccepted: ImportController.importFiles(importDialog.selectedFiles)
    }

    AppSlider {
        id: sizeSlider

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingXl
        anchors.verticalCenter: search.verticalCenter
        from: Theme.coverTileSmallest
        to: Theme.coverTileLargest
        value: settings.tileWidth
        leftIconName: "gridSmall"
        rightIconName: "library"
        Accessible.name: qsTr("Cover-Größe")

        // valueChanged (not moved), so keyboard and accessibility tools work too.
        onValueChanged: settings.tileWidth = sizeSlider.value
    }

    // --- format selection and sorting ----------------------------------------
    ChipGroup {
        id: formats

        anchors.left: parent.left
        anchors.top: search.bottom
        anchors.leftMargin: Theme.spacingXl
        anchors.topMargin: Theme.spacingM

        Chip {
            text: qsTr("Alle")
            count: works.countAll
            checked: settings.formatFilter === WorkFilterModel.AllFormats
            onClicked: settings.formatFilter = WorkFilterModel.AllFormats
        }
        Chip {
            text: qsTr("E-Books")
            icons: [{ name: "book", color: Theme.read }]
            count: works.countEbook
            checked: settings.formatFilter === WorkFilterModel.Ebook
            onClicked: settings.formatFilter = WorkFilterModel.Ebook
        }
        Chip {
            text: qsTr("Hörbücher")
            icons: [{ name: "headphones", color: Theme.listen }]
            count: works.countAudiobook
            checked: settings.formatFilter === WorkFilterModel.Audiobook
            onClicked: settings.formatFilter = WorkFilterModel.Audiobook
        }
        Chip {
            text: qsTr("Beides")
            icons: [{ name: "book", color: Theme.read }, { name: "headphones", color: Theme.listen }]
            count: works.countBoth
            checked: settings.formatFilter === WorkFilterModel.BothFormats
            onClicked: settings.formatFilter = WorkFilterModel.BothFormats
        }
    }

    // Stacks for series or every volume on its own. Not where the view
    // itself is about series.
    SelectMenu {
        anchors.right: sortMenu.left
        anchors.rightMargin: Theme.spacingS
        anchors.verticalCenter: formats.verticalCenter
        visible: !root.seriesOnly && !root.authorsOnly && !root.newOnly && root.openedSeries === ""
        iconName: "layers"
        label: qsTr("Gruppieren")
        options: [qsTr("Keine"), qsTr("Serien")]
        currentIndex: settings.groupSeries ? 1 : 0

        onActivated: function(index) {
            settings.groupSeries = index === 1
        }
    }

    // The option order matches WorkFilterModel.SortOrder (LastOpened, Title, Author).
    SelectMenu {
        id: sortMenu

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingXl
        anchors.verticalCenter: formats.verticalCenter
        // "Neu" is always newest first; "Autoren" has its own order.
        visible: !root.newOnly && !root.authorsOnly
        iconName: "sort"
        label: qsTr("Sortieren")
        options: [qsTr("Zuletzt geöffnet"), qsTr("Titel"), qsTr("Autor")]
        currentIndex: settings.sortOrder

        onActivated: function(index) {
            settings.sortOrder = index
        }
    }

    // "Autoren": stacks or shelves, instead of sorting.
    SelectMenu {
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingXl
        anchors.verticalCenter: formats.verticalCenter
        visible: root.authorList
        iconName: settings.authorLayout === 1 ? "list" : "layers"
        label: qsTr("Anordnung")
        options: [qsTr("Stapel"), qsTr("Alle Titel")]
        currentIndex: settings.authorLayout

        onActivated: function(index) {
            works.clearSelection()
            settings.authorLayout = index
        }
    }

    // Above the covers, scrolling with them: "Weitermachen" and the heading.
    Component {
        id: viewHeader

        Column {
            width: root.width - 2 * Theme.spacingXl
            topPadding: Theme.spacingL
            // Below the heading: also room for the sheets of series stacks.
            bottomPadding: Theme.spacingM + Theme.coverStackOffset * 2
            spacing: Theme.spacingXl

            ContinueRow {
                width: parent.width
                visible: root.showContinue
                model: continueWorks
                subtitle: settings.formatFilter === WorkFilterModel.Ebook
                          || settings.formatFilter === WorkFilterModel.Audiobook
                          ? "" : qsTr("gemeinsam für Lesen und Hören")

                onOpenRequested: function(workId, coverUrl, coverIsSquare) {
                    detail.openFor(workId, coverUrl, coverIsSquare)
                }
                onContinueRequested: function(workId, kind) {
                    root.continueWith(workId, kind)
                }
                onRemoveRequested: function(workId, kind) {
                    LibraryController.removeFromContinue(workId, kind)
                }
            }

            Item {
                width: parent.width
                height: Math.max(heading.implicitHeight, selectAll.implicitHeight)

                // Back from an opened series to the library (or all series,
                // or its author), from an opened author to all authors.
                LinkButton {
                    id: back

                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.openedSeries !== "" || root.openedAuthor !== ""
                    width: visible ? implicitWidth + Theme.spacingM : 0
                    iconName: "chevronLeft"
                    text: root.openedSeries !== "" && root.openedAuthor !== "" ? root.openedAuthor : root.title
                    font.pixelSize: Theme.fontBody
                    onClicked: root.goBack()
                }
                // Not over the list of all authors: the sidebar already says it.
                Text {
                    id: heading

                    anchors.left: back.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !root.authorList
                    width: visible ? implicitWidth : 0
                    text: root.openedSeries !== "" ? root.openedSeries
                          : root.openedAuthor !== "" ? root.openedAuthor : root.title
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                }
                Text {
                    anchors.left: heading.right
                    anchors.leftMargin: heading.visible ? Theme.spacingM : 0
                    anchors.right: selectAll.left
                    anchors.rightMargin: Theme.spacingM
                    anchors.baseline: heading.baseline
                    elide: Text.ElideRight
                    text: {
                        const count = root.openedSeries !== "" ? qsTr("%Ln Bände", "", works.count)
                                    : root.seriesOnly ? qsTr("%Ln Serien", "", works.count)
                                    : root.authorList ? qsTr("%Ln Autoren", "", works.count)
                                    : root.openedAuthor !== "" || root.newOnly
                                      ? qsTr("%Ln Titel", "", works.workCount)
                                    : root.filtered ? qsTr("%L1 von %Ln Titeln", "", root.total).arg(works.workCount)
                                                    : qsTr("%Ln Titel", "", root.total)
                        return works.selectedCount > 0
                               ? count + " · " + qsTr("%Ln ausgewählt", "", works.selectedCount) : count
                    }
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                }
                Row {
                    id: selectAll

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.canSelect
                    spacing: Theme.spacingS

                    LinkButton {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Alle auswählen")
                        onClicked: works.selectAll()
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Strg A")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }
    }

    // --- cover grid, with "Weitermachen" and the heading as its header --------
    GridView {
        id: grid

        // Columns: as many as fit with at least the chosen tile width each.
        readonly property int gap: Theme.spacingL
        readonly property int columns: Math.max(2, Math.floor((width + gap) / (settings.tileWidth + gap)))
        readonly property real tileWidth: cellWidth - gap

        anchors.top: formats.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spacingS
        anchors.leftMargin: Theme.spacingXl
        // The gap of the last column is part of the cell, so subtract it here.
        anchors.rightMargin: Theme.spacingXl - gap

        visible: !root.authorShelves
        cellWidth: Math.floor(width / columns)
        cellHeight: Math.round(tileWidth * Theme.coverAspect) + Theme.coverTileTextHeight + Theme.spacingL
        bottomMargin: Theme.spacingXl
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: cellHeight * 2
        // Not while the shelves show the works.
        model: root.authorShelves ? null : works

        ScrollBar.vertical: ScrollBar {}

        header: viewHeader

        delegate: Item {
            id: cell

            required property string workId
            required property string title
            required property string authorText
            required property url coverUrl
            required property bool coverIsSquare
            required property bool hasEbook
            required property bool hasAudiobook
            required property real readProgress
            required property real listenProgress
            required property int workState
            required property bool selected
            required property bool canSplit
            required property string series
            required property int seriesCount
            required property int seriesFirst
            required property int seriesLast
            required property string stackName

            // The volumes of a series (or the works of an author) as one tile.
            readonly property bool stack: cell.seriesCount > 0
            readonly property bool authorStack: cell.stack && root.authorList

            width: grid.cellWidth
            height: grid.cellHeight

            CoverTile {
                id: tile

                dropLabel: drop.containsDrag && drop.targetWorkId === cell.workId
                           ? drop.labelFor(cell.title) : ""
                width: grid.tileWidth
                title: cell.stack ? cell.stackName : cell.title
                author: cell.authorStack ? qsTr("%Ln Titel · Autor", "", cell.seriesCount)
                        : cell.stack ? qsTr("%Ln Bände · Serie", "", cell.seriesCount) : cell.authorText
                stacked: cell.stack
                stackLabel: !cell.stack || cell.seriesFirst === 0 ? ""
                            : cell.seriesFirst === cell.seriesLast ? qsTr("Band %1").arg(cell.seriesFirst)
                            : qsTr("Band %1–%2").arg(cell.seriesFirst).arg(cell.seriesLast)
                selectable: true
                selected: cell.selected
                selecting: works.selectedCount > 0
                coverUrl: cell.coverUrl
                coverIsSquare: cell.coverIsSquare
                hasEbook: cell.hasEbook
                hasAudiobook: cell.hasAudiobook
                readProgress: cell.readProgress
                listenProgress: cell.listenProgress
                workState: cell.workState

                // While titles are selected a click selects (phase 13).
                onClicked: {
                    if (works.selectedCount > 0)
                        works.toggleSelected(cell.workId)
                    else if (cell.authorStack)
                        root.openAuthor(cell.stackName)
                    else if (cell.stack)
                        root.openSeries(cell.stackName)
                    else
                        detail.openFor(cell.workId, cell.coverUrl, cell.coverIsSquare)
                }
                onSelectToggled: works.toggleSelected(cell.workId)
                onMenuRequested: function(x, y) {
                    root.openWorkMenu(tile, x, y, {
                        workId: cell.workId, title: cell.title,
                        series: cell.stack ? cell.stackName : cell.series,
                        coverUrl: cell.coverUrl, coverIsSquare: cell.coverIsSquare,
                        stack: cell.stack, authorStack: cell.authorStack,
                        hasEbook: cell.hasEbook, hasAudiobook: cell.hasAudiobook,
                        readProgress: cell.readProgress, listenProgress: cell.listenProgress,
                        workState: cell.workState, selected: cell.selected, canSplit: cell.canSplit
                    })
                }
                onReadRequested: root.continueWith(cell.workId, "ebook")
                onListenRequested: root.continueWith(cell.workId, "audiobook")
            }
        }
    }

    // --- "Autoren" as shelves: each author with all its works below -----------
    // One row per author (the stacks of works); each row has its own list of
    // the author's works, made only while the row is in view.
    ListView {
        id: shelves

        anchors.fill: grid
        // Like the grid, whose last column ends with a gap.
        anchors.rightMargin: grid.gap
        visible: root.authorShelves
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        bottomMargin: Theme.spacingXl
        spacing: Theme.spacingXl
        model: root.authorShelves ? works : null
        header: viewHeader

        ScrollBar.vertical: ScrollBar {
            parent: shelves.parent
            anchors.top: shelves.top
            anchors.bottom: shelves.bottom
            anchors.left: shelves.right
        }

        delegate: Column {
            id: shelf

            required property string stackName
            required property int seriesCount

            width: ListView.view ? ListView.view.width : 0
            spacing: Theme.spacingM

            // The author, how many titles, and an accent line to the right edge.
            Item {
                width: parent.width
                height: authorName.implicitHeight

                Text {
                    id: authorName

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, parent.width * 0.7)
                    elide: Text.ElideRight
                    text: shelf.stackName
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Bold
                }
                Text {
                    id: titleCount

                    anchors.left: authorName.right
                    anchors.leftMargin: Theme.spacingM
                    anchors.baseline: authorName.baseline
                    text: qsTr("%Ln Titel", "", shelfWorks.count)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                }
                Rectangle {
                    anchors.left: titleCount.right
                    anchors.leftMargin: Theme.spacingL
                    anchors.right: parent.right
                    anchors.verticalCenter: authorName.verticalCenter
                    height: Theme.shelfLineWidth
                    radius: height / 2
                    color: Theme.accent
                }
            }

            // The works one by one: series and their volumes in order.
            WorkFilterModel {
                id: shelfWorks

                sourceModel: LibraryController.works
                authorFilter: shelf.stackName
                formatFilter: settings.formatFilter
                searchText: search.text
                sortOrder: WorkFilterModel.Series
            }

            Flow {
                width: parent.width
                spacing: grid.gap

                Repeater {
                    model: shelfWorks

                    delegate: CoverTile {
                        id: shelfTile

                        required property string workId
                        required property string authorText
                        required property bool canSplit
                        required property string series
                        // Not required in CoverTile, but taken from the model here.
                        required workState

                        width: grid.tileWidth
                        author: shelfTile.authorText
                        selectable: true
                        // The selection is that of the view (works).
                        selected: works.selectedCount >= 0 && works.isSelected(shelfTile.workId)
                        selecting: works.selectedCount > 0

                        onClicked: {
                            if (works.selectedCount > 0)
                                works.toggleWork(shelfTile.workId)
                            else
                                detail.openFor(shelfTile.workId, shelfTile.coverUrl, shelfTile.coverIsSquare)
                        }
                        onSelectToggled: works.toggleWork(shelfTile.workId)
                        onMenuRequested: function(x, y) {
                            root.openWorkMenu(shelfTile, x, y, {
                                workId: shelfTile.workId, title: shelfTile.title, series: shelfTile.series,
                                coverUrl: shelfTile.coverUrl, coverIsSquare: shelfTile.coverIsSquare,
                                stack: false, authorStack: false,
                                hasEbook: shelfTile.hasEbook, hasAudiobook: shelfTile.hasAudiobook,
                                readProgress: shelfTile.readProgress, listenProgress: shelfTile.listenProgress,
                                workState: shelfTile.workState, selected: shelfTile.selected,
                                canSplit: shelfTile.canSplit
                            })
                        }
                        onReadRequested: root.continueWith(shelfTile.workId, "ebook")
                        onListenRequested: root.continueWith(shelfTile.workId, "audiobook")
                    }
                }
            }
        }
    }

    DetailDrawer {
        id: detail

        onEditRequested: function(workId) {
            metadataEditor.openFor(workId)
        }
    }

    MetadataEditor {
        id: metadataEditor

        navigationModel: works
    }

    // --- selection: bar and questions ----------------------------------------
    BulkActionBar {
        anchors.horizontalCenter: grid.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.spacingL
        visible: works.selectedCount > 0
        count: works.selectedCount
        canMerge: works.canMergeSelection
        // An import that is running may still add to the selected works.
        busy: ImportController.busy
        exporting: ExportController.running

        onMetadataRequested: root.editSelection()
        onMergeRequested: root.askMerge()
        onSeriesRequested: root.askSeries()
        onExportRequested: root.askExport()
        onFinishedRequested: LibraryController.markWorks(works.selectedIds, true)
        onNewRequested: root.askMarkNew()
        onRemoveRequested: root.askRemove(works.selectedIds, "")
        onCloseRequested: works.clearSelection()
    }

    FolderDialog {
        id: exportDialog

        // The works to export, taken when the dialog opens.
        property list<string> workIds

        title: qsTr("Wohin sollen die Dateien exportiert werden?")
        acceptLabel: qsTr("Hierhin exportieren")
        onAccepted: ExportController.exportWorks(exportDialog.workIds, exportDialog.selectedFolder)
    }

    // --- context menu of a cover (right click) --------------------------------
    WorkMenu {
        id: workMenu

        // The title that was clicked (its name is the "title" of the menu).
        property string workId
        // Its series, or the name of the stack (series or author).
        property string series
        property url coverUrl
        property bool coverIsSquare: false

        selectionCount: works.selectedCount
        canMerge: works.canMergeSelection
        busy: ImportController.busy
        exporting: ExportController.running

        onOpenRequested: {
            if (workMenu.authorStack)
                root.openAuthor(workMenu.series)
            else if (workMenu.stack)
                root.openSeries(workMenu.series)
            else
                detail.openFor(workMenu.workId, workMenu.coverUrl, workMenu.coverIsSquare)
        }
        onReadRequested: root.continueWith(workMenu.workId, "ebook")
        onOpenExternallyRequested: LibraryController.openFormat(workMenu.workId, "ebook")
        onListenRequested: root.continueWith(workMenu.workId, "audiobook")
        onEditRequested: metadataEditor.openFor(workMenu.workId)
        onFinishedToggled: function(kind) {
            const finished = kind === "audiobook" ? workMenu.audiobookFinished : workMenu.ebookFinished
            LibraryController.setFinished(workMenu.workId, kind, !finished)
        }
        // A stack: all its works; a title (also one on a shelf): only it.
        onSelectToggled: {
            if (workMenu.stack)
                works.toggleSelected(workMenu.workId)
            else
                works.toggleWork(workMenu.workId)
        }
        onClearSelectionRequested: works.clearSelection()
        onMergeRequested: root.askMerge()
        onSeriesRequested: root.askSeries()
        onMarkRequested: function(finished) {
            if (finished)
                LibraryController.markWorks(works.selectedIds, true)
            else
                root.askMarkNew()
        }
        onSplitRequested: LibraryController.splitWork(workMenu.workId)
        onSeriesClearRequested: LibraryController.clearSeries(works.selectedIds)
        onSeriesRemoveRequested: {
            root.actionTitle = workMenu.series
            removeSeriesDialog.open()
        }
        onExportRequested: root.askExport()
        onRemoveRequested: {
            if (workMenu.bulk)
                root.askRemove(works.selectedIds, "")
            else
                root.askRemove([workMenu.workId], workMenu.title)
        }
    }

    // actionTitle: the work that is left, see LibraryController.mergeTitle().
    ConfirmDialog {
        id: mergeDialog

        title: root.actionIds.length > 1 ? qsTr("%Ln Titel kombinieren?", "", root.actionIds.length)
                                         : qsTr("Hörbuch-Dateien verbinden?")
        message: root.actionIds.length > 1
                 ? qsTr("Sie werden ein Eintrag „%1“, der sich am Stück lesen und hören lässt: E-Books als Bände nach ihrer Bandnummer, Hörbücher nacheinander. Mit „Kombination auflösen“ (Rechtsklick) werden es wieder einzelne Titel.").arg(root.actionTitle)
                 : qsTr("Die Hörbuch-Dateien von „%1“ werden zu einem Hörbuch mit einem Kapitel pro Datei. Das lässt sich nicht rückgängig machen.").arg(root.actionTitle)
        confirmText: root.actionIds.length > 1 ? qsTr("Kombinieren") : qsTr("Verbinden")

        onAccepted: {
            works.clearSelection()
            LibraryController.mergeWorks(root.actionIds)
        }
    }

    // Titles without volume numbers (or with gaps or doubles): the user
    // gives the order first, see SeriesOrderDialog.
    SeriesDialog {
        id: seriesDialog

        suggestions: LibraryController.seriesNames

        onAccepted: {
            if (LibraryController.seriesNeedsOrder(root.actionIds, seriesDialog.name))
                seriesOrderDialog.openFor(root.actionIds, seriesDialog.name, settings.formatFilter)
            else
                LibraryController.setSeries(root.actionIds, seriesDialog.name)
        }
    }

    SeriesOrderDialog {
        id: seriesOrderDialog
    }

    // actionTitle: the series that is dissolved.
    ConfirmDialog {
        id: removeSeriesDialog

        title: qsTr("Serie „%1“ auflösen?").arg(root.actionTitle)
        message: qsTr("Die Bände bleiben in der Bibliothek, verlieren aber den Seriennamen und ihre Bandnummer.")
        confirmText: qsTr("Auflösen")

        onAccepted: LibraryController.removeSeries(root.actionTitle)
    }

    ConfirmDialog {
        id: markNewDialog

        title: qsTr("%Ln Titel als neu markieren?", "", root.actionIds.length)
        message: qsTr("Fortschritt, Lese- und Hörpositionen gehen verloren. Die Titel erscheinen wieder als „Neu“ und nicht mehr unter „Weitermachen“.")
        confirmText: qsTr("Als neu markieren")
        destructive: true

        onAccepted: LibraryController.markWorks(root.actionIds, false)
    }

    // actionTitle: the one title that is removed, "" for the selection;
    // actionKind: only that format.
    ConfirmDialog {
        id: removeDialog

        title: {
            const one = root.actionTitle !== ""
            if (root.actionKind === "ebook")
                return one ? qsTr("E-Book von „%1“ entfernen?").arg(root.actionTitle)
                           : qsTr("E-Books von %Ln Titeln entfernen?", "", root.actionIds.length)
            if (root.actionKind === "audiobook")
                return one ? qsTr("Hörbuch von „%1“ entfernen?").arg(root.actionTitle)
                           : qsTr("Hörbücher von %Ln Titeln entfernen?", "", root.actionIds.length)
            return one ? qsTr("„%1“ entfernen?").arg(root.actionTitle)
                       : qsTr("%Ln Titel entfernen?", "", root.actionIds.length)
        }
        message: {
            if (root.actionKind === "ebook") {
                return LibraryController.canImport
                       ? qsTr("Nur die E-Books werden entfernt, Hörbücher derselben Titel bleiben. Die Dateien im Bibliotheksordner kommen in den Papierkorb.")
                       : qsTr("Nur die E-Books werden entfernt, Hörbücher derselben Titel bleiben.")
            }
            if (root.actionKind === "audiobook") {
                return LibraryController.canImport
                       ? qsTr("Nur die Hörbücher werden entfernt, E-Books derselben Titel bleiben. Die Dateien im Bibliotheksordner kommen in den Papierkorb.")
                       : qsTr("Nur die Hörbücher werden entfernt, E-Books derselben Titel bleiben.")
            }
            if (root.actionTitle !== "") {
                return LibraryController.canImport
                       ? qsTr("Der Titel wird aus der Bibliothek entfernt. Seine Dateien im Bibliotheksordner kommen in den Papierkorb.")
                       : qsTr("Der Titel wird aus der Bibliothek entfernt.")
            }
            return LibraryController.canImport
                   ? qsTr("Die Titel werden aus der Bibliothek entfernt. Ihre Dateien im Bibliotheksordner kommen in den Papierkorb.")
                   : qsTr("Die Titel werden aus der Bibliothek entfernt.")
        }
        confirmText: qsTr("Entfernen")
        destructive: true

        onAccepted: {
            if (root.actionOnSelection)
                works.clearSelection()
            LibraryController.removeWorks(root.actionIds, root.actionKind)
        }
    }

    // --- empty states --------------------------------------------------------
    Column {
        anchors.horizontalCenter: grid.horizontalCenter
        anchors.top: grid.top
        anchors.topMargin: (grid.headerItem ? grid.headerItem.height : 0) + Theme.spacingXxl * 2
        spacing: Theme.spacingL
        visible: works.count === 0

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.total === 0 ? qsTr("Noch keine Titel in der Bibliothek.")
                  : search.text !== "" ? qsTr("Keine Treffer für „%1“.").arg(search.text)
                  : root.noSeries ? qsTr("Noch keine Serien. Eine Serie entsteht, sobald sie in den Metadaten eines Titels steht.")
                  : root.noAuthors ? qsTr("Noch keine Autoren. Ein Autor erscheint, sobald er in den Metadaten eines Titels steht.")
                  : root.noNew ? qsTr("Keine neuen Titel. Hier erscheint, was in den letzten %Ln Minuten hinzugefügt wurde.", "", root.newMinutes)
                               : qsTr("Keine Titel in dieser Auswahl.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
        }
        AppButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.total > 0 && !root.noSeries && !root.noAuthors && !root.noNew
            text: qsTr("Filter zurücksetzen")
            iconName: "x"
            onClicked: root.resetFilters()
        }
        AppButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.total === 0 && LibraryController.canImport
            text: qsTr("E-Books und Hörbücher hinzufügen")
            iconName: "plus"
            variant: AppButton.Primary
            onClicked: importDialog.open()
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(implicitWidth, grid.width - 2 * Theme.spacingXl)
            visible: root.total === 0 && LibraryController.canImport
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Oder Dateien und ganze Ordner in dieses Fenster ziehen. Sie werden in die Bibliothek kopiert:\n%1")
                      .arg(LibraryController.libraryRoot)
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            lineHeight: Theme.lineHeightText
        }
    }

    // --- drag & drop (view D) -------------------------------------------------
    DropArea {
        id: drop

        // What is dragged (see ImportController.inspect()).
        property var summary: ({})
        // The work under the pointer, empty = free space.
        property string targetWorkId
        // -1 / 1 while the pointer is near the top / bottom edge of the grid.
        property int scrollDirection: 0
        property point lastPosition

        readonly property string summaryText: drop.summary.name
                                              ? drop.summary.name + " · " + drop.summary.typesText : ""

        // Text on the cover the files would be added to.
        function labelFor(title: string): string {
            const ebooks = drop.summary.ebookCount || 0
            const audiobooks = drop.summary.audiobookCount || 0
            if (audiobooks > 0 && ebooks === 0)
                return qsTr("Als Hörbuch zu „%1“ hinzufügen").arg(title)
            if (ebooks > 0 && audiobooks === 0)
                return qsTr("Als E-Book zu „%1“ hinzufügen").arg(title)
            return qsTr("Zu „%1“ hinzufügen").arg(title)
        }

        // Finds the cover under the point (in the coordinates of the drop area).
        function updateTarget(x: real, y: real) {
            drop.lastPosition = Qt.point(x, y)
            const view = root.activeView
            const p = view.mapFromItem(drop, x, y)
            const inGrid = p.x >= 0 && p.y >= 0 && p.x < view.width && p.y < view.height
            drop.scrollDirection = !inGrid ? 0
                                 : p.y < Theme.dropScrollMargin ? -1
                                 : p.y > view.height - Theme.dropScrollMargin ? 1 : 0

            let workId = ""
            // On the shelves files always become new works.
            if (inGrid && !root.authorShelves) {
                const contentX = p.x + grid.contentX
                const contentY = p.y + grid.contentY
                const index = grid.indexAt(contentX, contentY)
                // The gap between two columns is free space, and so is a
                // series stack: it stands for several works.
                const xInCell = contentX - Math.floor(contentX / grid.cellWidth) * grid.cellWidth
                if (index >= 0 && xInCell < grid.tileWidth && !works.isStack(index))
                    workId = works.workIdAt(index)
            }
            drop.targetWorkId = workId
        }

        function reset() {
            drop.summary = ({})
            drop.targetWorkId = ""
            drop.scrollDirection = 0
        }

        anchors.fill: parent
        enabled: LibraryController.canImport && !root.popupOpen

        onEntered: function(drag) {
            drop.summary = drag.hasUrls ? ImportController.inspect(drag.urls) : ({})
            if (!drop.summary.fileCount) {
                drag.accepted = false // nothing calibly can import
                return
            }
            drag.accept(Qt.CopyAction)
            drop.updateTarget(drag.x, drag.y)
        }
        onPositionChanged: function(drag) {
            drop.updateTarget(drag.x, drag.y)
        }
        onExited: drop.reset()
        onDropped: function(event) {
            ImportController.importFiles(event.urls, drop.targetWorkId)
            event.accept(Qt.CopyAction)
            drop.reset()
        }
    }

    // Scrolls the grid while files are dragged near its top or bottom edge,
    // so every cover can be reached.
    Timer {
        interval: Theme.dropScrollInterval
        repeat: true
        running: drop.containsDrag && drop.scrollDirection !== 0

        onTriggered: {
            const view = root.activeView
            const top = view.originY - view.topMargin
            const bottom = Math.max(top, view.originY + view.contentHeight + view.bottomMargin - view.height)
            view.contentY = Math.max(top, Math.min(bottom, view.contentY + drop.scrollDirection * Theme.dropScrollStep))
            drop.updateTarget(drop.lastPosition.x, drop.lastPosition.y)
        }
    }

    DropHint {
        anchors.horizontalCenter: grid.horizontalCenter
        anchors.top: grid.top
        anchors.topMargin: Theme.spacingS
        width: Math.min(implicitWidth, grid.width)
        visible: drop.containsDrag
        onCover: drop.targetWorkId !== ""
        summary: drop.summaryText
    }

    // --- import queue ----------------------------------------------------------
    ImportQueue {
        id: importQueue

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacingL
        width: Math.min(implicitWidth, parent.width - 2 * Theme.spacingL)
        visible: ImportController.queue.count > 0
        model: ImportController.queue
        finishedCount: ImportController.queue.finishedCount

        onReviewRequested: function(entryId, workId) {
            ImportController.queue.markReviewed(entryId)
            metadataEditor.openFor(workId)
        }
        onEntryActivated: function(workId, coverUrl, coverIsSquare) {
            detail.openFor(workId, coverUrl, coverIsSquare)
        }
        onCloseRequested: ImportController.queue.clearFinished()

        // Rows that went fine disappear after a while; rows that need
        // attention stay until they are looked at or the queue is closed.
        Timer {
            interval: Theme.messageDuration
            running: importQueue.visible && !ImportController.queue.active
                     && ImportController.queue.attentionCount === 0

            onTriggered: ImportController.queue.clearFinished()
        }
    }
}

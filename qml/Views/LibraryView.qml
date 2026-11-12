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
// or removes them. Esc ends the selection, then leaves an opened series.
// A right click on a cover opens a menu with the actions for that title, or
// for the selection if the title is one of several selected ones.
//
// Properties:
//   seriesOnly  show only the series, each as a stack: the view "Serien" of
//               the sidebar (see SeriesView)
ViewBase {
    id: root

    property bool seriesOnly: false
    // The series whose volumes are shown, "" = all works: an opened stack.
    property string openedSeries: ""
    // The metadata editor steps through the selection instead of the grid.
    property bool editingSelection: false
    // The works the question of mergeDialog / removeDialog is about: the
    // selection (bar, menu of a selected title) or the title of the menu.
    property list<string> actionIds: []
    property string actionTitle
    property bool actionOnSelection: false

    readonly property int total: LibraryController.works.count
    readonly property bool filtered: works.workCount !== root.total
    // "Weitermachen" shows at most this many titles, and none while searching.
    readonly property int continueLimit: 12
    readonly property bool showContinue: continueWorks.count > 0 && search.text === ""
                                         && !root.seriesOnly && root.openedSeries === ""
    // The list of all series has only stacks, which cannot be selected.
    readonly property bool canSelect: works.count > 0 && !(root.seriesOnly && root.openedSeries === "")
    readonly property bool popupOpen: detail.visible || metadataEditor.visible
                                      || mergeDialog.visible || removeDialog.visible || workMenu.visible
    // The view "Serien" without any series (and without a filter that hides them).
    readonly property bool noSeries: root.seriesOnly && root.openedSeries === "" && search.text === ""
                                     && settings.formatFilter === WorkFilterModel.AllFormats

    // Back to the top of the grid, including its header.
    function scrollToTop() {
        grid.contentY = grid.originY - grid.topMargin
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

    // "Metadaten" of the bar: the editor with the first selected work;
    // ‹ / › there step through the selection.
    function editSelection() {
        const ids = works.selectedIds
        if (ids.length === 0)
            return
        selectedWorks.onlyIds = ids
        root.editingSelection = true
        metadataEditor.openFor(ids[0])
    }

    // Asks before the selected works are joined into one.
    function askMerge() {
        root.actionIds = works.selectedIds
        root.actionOnSelection = true
        root.actionTitle = LibraryController.mergeTitle(root.actionIds)
        mergeDialog.open()
    }

    // Asks before works are removed: the selection, or one title (with its name).
    function askRemove(workIds: list<string>, title: string) {
        root.actionIds = workIds
        root.actionOnSelection = title === ""
        root.actionTitle = title
        removeDialog.open()
    }

    // Choices that are kept between starts (stored by Qt in the user settings).
    Settings {
        id: settings

        category: "library"

        property int formatFilter: WorkFilterModel.AllFormats
        property int sortOrder: WorkFilterModel.LastOpened
        property real tileWidth: Theme.coverTileMinWidth
        property bool groupSeries: true
    }

    WorkFilterModel {
        id: works

        sourceModel: LibraryController.works
        formatFilter: settings.formatFilter
        sortOrder: settings.sortOrder
        searchText: search.text
        groupSeries: root.seriesOnly || settings.groupSeries
        seriesOnly: root.seriesOnly
        seriesFilter: root.openedSeries

        // Deferred: countChanged arrives while the GridView is still applying
        // the row changes; scrolling right away would create duplicate tiles.
        onCountChanged: Qt.callLater(root.scrollToTop)
        // The opened series is gone (its volumes were removed, merged or
        // renamed): back to where it was opened from.
        onWorkCountChanged: {
            if (root.openedSeries !== "" && works.workCount === 0 && search.text === ""
                    && settings.formatFilter === WorkFilterModel.AllFormats)
                Qt.callLater(root.closeSeries)
        }
    }

    // The format selection also applies to "Weitermachen".
    ContinueModel {
        id: continueWorks

        sourceModel: LibraryController.works
        formatFilter: settings.formatFilter
        limit: root.continueLimit
    }

    // The selected works in the shown order, for the metadata editor.
    WorkFilterModel {
        id: selectedWorks

        sourceModel: LibraryController.works
    }

    Shortcut {
        sequences: [StandardKey.SelectAll]
        enabled: root.canSelect && !root.popupOpen
        onActivated: works.selectAll()
    }
    Shortcut {
        sequence: "Esc"
        enabled: !root.popupOpen && !search.activeFocus
                 && (works.selectedCount > 0 || root.openedSeries !== "")
        onActivated: {
            if (works.selectedCount > 0)
                works.clearSelection()
            else
                root.closeSeries()
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
        visible: !root.seriesOnly && root.openedSeries === ""
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
        iconName: "sort"
        label: qsTr("Sortieren")
        options: [qsTr("Zuletzt geöffnet"), qsTr("Titel"), qsTr("Autor")]
        currentIndex: settings.sortOrder

        onActivated: function(index) {
            settings.sortOrder = index
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

        cellWidth: Math.floor(width / columns)
        cellHeight: Math.round(tileWidth * Theme.coverAspect) + Theme.coverTileTextHeight + Theme.spacingL
        bottomMargin: Theme.spacingXl
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: cellHeight * 2
        model: works

        ScrollBar.vertical: ScrollBar {}

        header: Column {
            width: grid.width - grid.gap
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
            }

            Item {
                width: parent.width
                height: Math.max(heading.implicitHeight, selectAll.implicitHeight)

                // Back from an opened series to the library (or all series).
                LinkButton {
                    id: back

                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.openedSeries !== ""
                    width: visible ? implicitWidth + Theme.spacingM : 0
                    iconName: "chevronLeft"
                    text: root.title
                    font.pixelSize: Theme.fontBody
                    onClicked: root.closeSeries()
                }
                Text {
                    id: heading

                    anchors.left: back.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.openedSeries !== "" ? root.openedSeries : root.title
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                }
                Text {
                    anchors.left: heading.right
                    anchors.leftMargin: Theme.spacingM
                    anchors.right: selectAll.left
                    anchors.rightMargin: Theme.spacingM
                    anchors.baseline: heading.baseline
                    elide: Text.ElideRight
                    text: {
                        const count = root.openedSeries !== "" ? qsTr("%Ln Bände", "", works.count)
                                    : root.seriesOnly ? qsTr("%Ln Serien", "", works.count)
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
            required property string series
            required property int seriesCount
            required property int seriesFirst
            required property int seriesLast

            // The volumes of a series as one tile.
            readonly property bool stack: cell.seriesCount > 0

            width: grid.cellWidth
            height: grid.cellHeight

            CoverTile {
                id: tile

                dropLabel: drop.containsDrag && drop.targetWorkId === cell.workId
                           ? drop.labelFor(cell.title) : ""
                width: grid.tileWidth
                title: cell.stack ? cell.series : cell.title
                author: cell.stack ? qsTr("%Ln Bände · Serie", "", cell.seriesCount) : cell.authorText
                stacked: cell.stack
                stackLabel: !cell.stack || cell.seriesFirst === 0 ? ""
                            : cell.seriesFirst === cell.seriesLast ? qsTr("Band %1").arg(cell.seriesFirst)
                            : qsTr("Band %1–%2").arg(cell.seriesFirst).arg(cell.seriesLast)
                selectable: !cell.stack
                selected: cell.selected
                selecting: works.selectedCount > 0
                coverUrl: cell.coverUrl
                coverIsSquare: cell.coverIsSquare
                hasEbook: cell.hasEbook
                hasAudiobook: cell.hasAudiobook
                readProgress: cell.readProgress
                listenProgress: cell.listenProgress
                workState: cell.workState

                onClicked: {
                    if (cell.stack)
                        root.openSeries(cell.series)
                    else
                        detail.openFor(cell.workId, cell.coverUrl, cell.coverIsSquare)
                }
                onSelectToggled: works.toggleSelected(cell.workId)
                onMenuRequested: function(x, y) {
                    workMenu.workId = cell.workId
                    workMenu.title = cell.title
                    workMenu.series = cell.series
                    workMenu.coverUrl = cell.coverUrl
                    workMenu.coverIsSquare = cell.coverIsSquare
                    workMenu.stack = cell.stack
                    workMenu.hasEbook = cell.hasEbook
                    workMenu.readsInApp = cell.hasEbook && ReaderController.canRead(cell.workId)
                    workMenu.hasAudiobook = cell.hasAudiobook
                    workMenu.ebookFinished = cell.readProgress >= 1
                    workMenu.audiobookFinished = cell.listenProgress >= 1
                    workMenu.fileMissing = cell.workState === WorkState.FileMissing
                    workMenu.selected = cell.selected
                    workMenu.popup(tile, x, y)
                }
                onReadRequested: root.continueWith(cell.workId, "ebook")
                onListenRequested: root.continueWith(cell.workId, "audiobook")
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

        navigationModel: root.editingSelection ? selectedWorks : works

        onClosed: root.editingSelection = false
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

        onMetadataRequested: root.editSelection()
        onMergeRequested: root.askMerge()
        onRemoveRequested: root.askRemove(works.selectedIds, "")
        onCloseRequested: works.clearSelection()
    }

    // --- context menu of a cover (right click) --------------------------------
    WorkMenu {
        id: workMenu

        // The title that was clicked (its name is the "title" of the menu).
        property string workId
        property string series
        property url coverUrl
        property bool coverIsSquare: false

        selectionCount: works.selectedCount
        canMerge: works.canMergeSelection
        busy: ImportController.busy

        onOpenRequested: {
            if (workMenu.stack)
                root.openSeries(workMenu.series)
            else
                detail.openFor(workMenu.workId, workMenu.coverUrl, workMenu.coverIsSquare)
        }
        onReadRequested: root.continueWith(workMenu.workId, "ebook")
        onOpenExternallyRequested: LibraryController.openFormat(workMenu.workId, "ebook")
        onListenRequested: root.continueWith(workMenu.workId, "audiobook")
        onEditRequested: {
            if (workMenu.bulk)
                root.editSelection()
            else
                metadataEditor.openFor(workMenu.workId)
        }
        onFinishedToggled: function(kind) {
            const finished = kind === "audiobook" ? workMenu.audiobookFinished : workMenu.ebookFinished
            LibraryController.setFinished(workMenu.workId, kind, !finished)
        }
        onSelectToggled: works.toggleSelected(workMenu.workId)
        onClearSelectionRequested: works.clearSelection()
        onMergeRequested: root.askMerge()
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

        title: root.actionIds.length > 1 ? qsTr("%Ln Titel zu einem Werk verbinden?", "", root.actionIds.length)
                                         : qsTr("Hörbuch-Dateien verbinden?")
        message: root.actionIds.length > 1
                 ? qsTr("Alle Formate kommen zu „%1“. Mehrere Hörbuch-Dateien werden dabei zu einem Hörbuch. Das lässt sich nicht rückgängig machen.").arg(root.actionTitle)
                 : qsTr("Die Hörbuch-Dateien von „%1“ werden zu einem Hörbuch mit einem Kapitel pro Datei. Das lässt sich nicht rückgängig machen.").arg(root.actionTitle)
        confirmText: qsTr("Verbinden")

        onAccepted: {
            works.clearSelection()
            LibraryController.mergeWorks(root.actionIds)
        }
    }

    // actionTitle: the one title that is removed, "" for the selection.
    ConfirmDialog {
        id: removeDialog

        title: root.actionTitle !== "" ? qsTr("„%1“ entfernen?").arg(root.actionTitle)
                                       : qsTr("%Ln Titel entfernen?", "", root.actionIds.length)
        message: {
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
            LibraryController.removeWorks(root.actionIds)
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
                                  : qsTr("Keine Titel in dieser Auswahl.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
        }
        AppButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.total > 0 && !root.noSeries
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
            const p = grid.mapFromItem(drop, x, y)
            const inGrid = p.x >= 0 && p.y >= 0 && p.x < grid.width && p.y < grid.height
            drop.scrollDirection = !inGrid ? 0
                                 : p.y < Theme.dropScrollMargin ? -1
                                 : p.y > grid.height - Theme.dropScrollMargin ? 1 : 0

            let workId = ""
            if (inGrid) {
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
            const top = grid.originY - grid.topMargin
            const bottom = Math.max(top, grid.originY + grid.contentHeight + grid.bottomMargin - grid.height)
            grid.contentY = Math.max(top, Math.min(bottom, grid.contentY + drop.scrollDirection * Theme.dropScrollStep))
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

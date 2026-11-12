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
// View D: files dropped on free space become new works, files dropped on a
// cover become formats of that work (ImportController); the import queue
// at the bottom right shows how it goes.
ViewBase {
    id: root

    readonly property int total: LibraryController.works.count
    readonly property bool filtered: works.count !== root.total

    // Back to the first row, including the top margin of the grid.
    function scrollToTop() {
        grid.contentY = grid.originY - grid.topMargin
    }

    function resetFilters() {
        search.clear()
        settings.formatFilter = WorkFilterModel.AllFormats
    }

    // Choices that are kept between starts (stored by Qt in the user settings).
    Settings {
        id: settings

        category: "library"

        property int formatFilter: WorkFilterModel.AllFormats
        property int sortOrder: WorkFilterModel.LastOpened
        property real tileWidth: Theme.coverTileMinWidth
    }

    WorkFilterModel {
        id: works

        sourceModel: LibraryController.works
        formatFilter: settings.formatFilter
        sortOrder: settings.sortOrder
        searchText: search.text

        // Deferred: countChanged arrives while the GridView is still applying
        // the row changes; scrolling right away would create duplicate tiles.
        onCountChanged: Qt.callLater(root.scrollToTop)
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

    // The option order matches WorkFilterModel.SortOrder (LastOpened, Title, Author).
    SelectMenu {
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

    // --- heading -------------------------------------------------------------
    Text {
        id: heading

        anchors.left: parent.left
        anchors.top: formats.bottom
        anchors.leftMargin: Theme.spacingXl
        anchors.topMargin: Theme.spacingXl
        text: root.title
        color: Theme.textPrimary
        font.pixelSize: Theme.fontLarge
        font.weight: Font.Bold
    }
    Text {
        anchors.left: heading.right
        anchors.leftMargin: Theme.spacingM
        anchors.baseline: heading.baseline
        text: root.filtered ? qsTr("%L1 von %Ln Titeln", "", root.total).arg(works.count)
                            : qsTr("%Ln Titel", "", root.total)
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }

    // --- cover grid ----------------------------------------------------------
    GridView {
        id: grid

        // Columns: as many as fit with at least the chosen tile width each.
        readonly property int gap: Theme.spacingL
        readonly property int columns: Math.max(2, Math.floor((width + gap) / (settings.tileWidth + gap)))
        readonly property real tileWidth: cellWidth - gap

        anchors.top: heading.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spacingM
        anchors.leftMargin: Theme.spacingXl
        // The gap of the last column is part of the cell, so subtract it here.
        anchors.rightMargin: Theme.spacingXl - gap

        cellWidth: Math.floor(width / columns)
        cellHeight: Math.round(tileWidth * Theme.coverAspect) + Theme.coverTileTextHeight + Theme.spacingL
        topMargin: Theme.coverStackOffset * 2 // room for the sheets of series stacks
        bottomMargin: Theme.spacingXl
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: cellHeight * 2
        model: works

        ScrollBar.vertical: ScrollBar {}

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

            width: grid.cellWidth
            height: grid.cellHeight

            CoverTile {
                dropLabel: drop.containsDrag && drop.targetWorkId === cell.workId
                           ? drop.labelFor(cell.title) : ""
                width: grid.tileWidth
                title: cell.title
                author: cell.authorText
                coverUrl: cell.coverUrl
                coverIsSquare: cell.coverIsSquare
                hasEbook: cell.hasEbook
                hasAudiobook: cell.hasAudiobook
                readProgress: cell.readProgress
                listenProgress: cell.listenProgress

                onClicked: detail.openFor(cell.workId, cell.coverUrl, cell.coverIsSquare)
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

    // --- empty states --------------------------------------------------------
    Column {
        anchors.horizontalCenter: grid.horizontalCenter
        anchors.top: heading.bottom
        anchors.topMargin: Theme.spacingXxl * 3
        spacing: Theme.spacingL
        visible: works.count === 0

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.total === 0 ? qsTr("Noch keine Titel in der Bibliothek.")
                                   : search.text !== "" ? qsTr("Keine Treffer für „%1“.").arg(search.text)
                                                        : qsTr("Keine Titel in dieser Auswahl.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
        }
        AppButton {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.total > 0
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
                // The gap between two columns is free space.
                const xInCell = contentX - Math.floor(contentX / grid.cellWidth) * grid.cellWidth
                if (index >= 0 && xInCell < grid.tileWidth)
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
        enabled: LibraryController.canImport && !detail.visible && !metadataEditor.visible

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

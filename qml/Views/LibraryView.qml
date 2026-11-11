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
        enabled: !LibraryController.importing
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

        onAccepted: LibraryController.importFiles(importDialog.selectedFiles)
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
            visible: root.total === 0 && LibraryController.canImport && !LibraryController.importing
            text: qsTr("E-Books und Hörbücher hinzufügen")
            iconName: "plus"
            variant: AppButton.Primary
            onClicked: importDialog.open()
        }
    }

    // --- import progress and result ------------------------------------------
    Rectangle {
        id: importStatus

        property string summary

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.spacingL
        width: Math.min(Theme.searchFieldWidth + Theme.formatCardWidth, parent.width - 2 * Theme.spacingXl)
        height: statusColumn.implicitHeight + 2 * Theme.spacingM
        radius: Theme.radiusL
        color: Theme.surfaceRaised
        border.width: Theme.borderWidth
        border.color: Theme.border
        visible: LibraryController.importing || summaryTimer.running

        Accessible.role: Accessible.StatusBar
        Accessible.name: statusText.text

        Column {
            id: statusColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Theme.spacingM
            spacing: Theme.spacingS

            Text {
                id: statusText

                width: parent.width
                elide: Text.ElideMiddle
                color: Theme.textPrimary
                font.pixelSize: Theme.fontBody
                text: {
                    if (!LibraryController.importing)
                        return importStatus.summary
                    if (LibraryController.importTotal === 0)
                        return qsTr("Dateien werden gesucht …")
                    return qsTr("Importiere %1 von %2: %3").arg(Math.min(LibraryController.importDone + 1,
                                                                        LibraryController.importTotal))
                                                         .arg(LibraryController.importTotal)
                                                         .arg(LibraryController.importFileName)
                }
            }
            ProgressLine {
                width: parent.width
                visible: LibraryController.importing
                value: LibraryController.importTotal > 0
                       ? LibraryController.importDone / LibraryController.importTotal : 0
            }
        }

        Timer {
            id: summaryTimer

            interval: Theme.messageDuration
        }

        Connections {
            target: LibraryController

            function onImportFinished(added: int, skipped: int, failed: int) {
                const parts = [qsTr("%n Datei(en) hinzugefügt", "", added)]
                if (skipped > 0)
                    parts.push(qsTr("%n bereits vorhanden", "", skipped))
                if (failed > 0)
                    parts.push(qsTr("%n fehlgeschlagen", "", failed))
                importStatus.summary = parts.join(" · ")
                summaryTimer.restart()
            }
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Calibly.Theme
import Calibly.Components
import Calibly.Controllers

// View A of the mockup: all works as a cover grid. The number of columns
// follows the available width.
ViewBase {
    id: root

    readonly property int count: LibraryController.works.count

    Text {
        id: heading

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: Theme.spacingXl
        anchors.topMargin: Theme.spacingXl
        text: root.title
        color: Theme.textPrimary
        font.pixelSize: Theme.fontTitle
        font.weight: Font.Bold
    }
    Text {
        anchors.left: heading.right
        anchors.leftMargin: Theme.spacingM
        anchors.baseline: heading.baseline
        text: qsTr("%Ln Titel", "", root.count)
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }

    GridView {
        id: grid

        // Columns: as many as fit with at least Theme.coverTileMinWidth each.
        readonly property int gap: Theme.spacingL
        readonly property int columns: Math.max(2, Math.floor((width + gap) / (Theme.coverTileMinWidth + gap)))
        readonly property real tileWidth: cellWidth - gap

        anchors.top: heading.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spacingL
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
        model: LibraryController.works

        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            id: cell

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
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: root.count === 0
        text: qsTr("Noch keine Titel in der Bibliothek.")
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }
}

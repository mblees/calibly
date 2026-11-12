pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Calibly.Theme
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// The view "Weitermachen" of the sidebar: every title that was started and
// is not finished, one ContinueCard each. It uses the same model as the
// row "Weitermachen" in the library (ContinueModel), so it shows the same
// titles in the same order (most recently opened first) – but all of them,
// and independent of the format selection of the library.
// The round button of a card continues directly, a click on the card opens
// the detail view.
ViewBase {
    id: root

    // Continues reading / listening where the user stopped: audiobooks play
    // in calibly, EPUB, PDF and text books open in the reader, other ebooks
    // in the default application.
    function continueWith(workId: string, kind: string) {
        if (kind === "audiobook")
            PlayerController.play(workId)
        else
            ReaderController.read(workId)
    }

    ContinueModel {
        id: continueWorks

        sourceModel: LibraryController.works
    }

    // --- heading -------------------------------------------------------------
    Text {
        id: heading

        anchors.left: parent.left
        anchors.top: parent.top
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
        visible: continueWorks.count > 0
        text: qsTr("%Ln Titel", "", continueWorks.count)
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }

    // --- cards -----------------------------------------------------------------
    GridView {
        id: grid

        // Columns: as many as fit with at least the usual card width each.
        readonly property int gap: Theme.spacingM
        readonly property int columns: Math.max(1, Math.floor((width + gap) / (Theme.continueCardWidth + gap)))

        anchors.top: heading.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spacingL
        anchors.leftMargin: Theme.spacingXl
        // The gap of the last column is part of the cell, so subtract it here.
        anchors.rightMargin: Theme.spacingXl - gap

        cellWidth: Math.floor(width / columns)
        // Height of a card (cover + its margins) plus the gap below it.
        cellHeight: Math.round(Theme.continueCoverWidth * Theme.coverAspect) + 2 * Theme.spacingM + gap
        bottomMargin: Theme.spacingXl
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: continueWorks

        ScrollBar.vertical: ScrollBar {}

        delegate: ContinueCard {
            id: card

            required property string workId
            // The other roles of ContinueModel are properties of the card.
            required chapterNumber
            required remainingSeconds
            required page
            required pages
            required chapterTitle
            required hasOtherFormat
            required available
            required lastOpened

            width: grid.cellWidth - grid.gap

            onClicked: detail.openFor(card.workId, card.coverUrl, card.coverIsSquare)
            onContinueRequested: root.continueWith(card.workId, card.kind)
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
    }

    // --- empty state -----------------------------------------------------------
    Column {
        anchors.centerIn: parent
        spacing: Theme.spacingM
        visible: continueWorks.count === 0

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.iconName
            size: Theme.iconSizeLarge
            strokeWidth: 1.5
            color: Theme.textDisabled
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Noch nichts angefangen.")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Titel, die du liest oder hörst, stehen hier, bis sie fertig sind.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
        }
    }
}

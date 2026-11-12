pragma ComponentBehavior: Bound
import QtQuick
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Models

// Row "Weitermachen" at the top of the library (view A): the titles the
// user is reading or listening to, most recently opened first, one
// ContinueCard each. ‹ / › scroll when not all cards fit.
//
// Properties:
//   model     ContinueModel
//   subtitle  small text next to the heading (optional)
// Signals:
//   openRequested(string workId, url coverUrl, bool coverIsSquare)  a card was clicked
//   continueRequested(string workId, string kind)   the round button of a card;
//                                                   kind: "ebook" or "audiobook"
Item {
    id: root

    required property ContinueModel model
    property string subtitle

    signal openRequested(string workId, url coverUrl, bool coverIsSquare)
    signal continueRequested(string workId, string kind)

    // Scrolls by as many whole cards as fit, direction -1 = left, 1 = right.
    function scrollBy(direction: int) {
        const card = Theme.continueCardWidth + list.spacing
        const step = Math.max(1, Math.floor((list.width + list.spacing) / card)) * card
        const first = list.originX
        const last = Math.max(first, list.originX + list.contentWidth - list.width)
        scrollAnimation.to = Math.max(first, Math.min(last, list.contentX + direction * step))
        scrollAnimation.restart()
    }

    implicitHeight: heading.height + Theme.spacingM + list.height

    Text {
        id: heading

        text: qsTr("Weitermachen")
        color: Theme.textPrimary
        font.pixelSize: Theme.fontLarge
        font.weight: Font.Bold
    }
    Text {
        anchors.left: heading.right
        anchors.leftMargin: Theme.spacingM
        anchors.baseline: heading.baseline
        text: root.subtitle
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }

    Row {
        anchors.right: parent.right
        anchors.verticalCenter: heading.verticalCenter
        visible: list.contentWidth > list.width

        IconButton {
            iconName: "chevronLeft"
            text: qsTr("Zurück blättern")
            enabled: !list.atXBeginning
            opacity: enabled ? 1 : Theme.disabledOpacity
            onClicked: root.scrollBy(-1)
        }
        IconButton {
            iconName: "chevronRight"
            text: qsTr("Weiter blättern")
            enabled: !list.atXEnd
            opacity: enabled ? 1 : Theme.disabledOpacity
            onClicked: root.scrollBy(1)
        }
    }

    ListView {
        id: list

        anchors.top: heading.bottom
        anchors.topMargin: Theme.spacingM
        width: parent.width
        // Height of a card (cover + its margins), also before the first card exists.
        height: Math.round(Theme.continueCoverWidth * Theme.coverAspect) + 2 * Theme.spacingM
        orientation: ListView.Horizontal
        spacing: Theme.spacingM
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.model

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

            onClicked: root.openRequested(card.workId, card.coverUrl, card.coverIsSquare)
            onContinueRequested: root.continueRequested(card.workId, card.kind)
        }

        NumberAnimation {
            id: scrollAnimation

            target: list
            property: "contentX"
            duration: Theme.durationDrawer
            easing.type: Easing.OutCubic
        }
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// One title to continue with (row "Weitermachen" of the library): cover,
// where the user stopped, progress and a round button that continues
// directly (orange = listen, blue = read). Presentation only.
//
// Properties:
//   title
//   kind              "ebook" or "audiobook": the format to continue with
//   coverUrl, coverIsSquare   cover image, see CoverArt
//   progress          0.0 – 1.0
//   lastOpened        shown as "zuletzt: heute, 08:14"; invalid = not shown
//   chapterNumber     audiobooks: current chapter, 1-based; 0 = unknown
//   remainingSeconds  audiobooks: time left, 0 = unknown
//   page, pages       ebooks: current page of pages, 0 = unknown
//   chapterTitle      ebooks: the chapter where the user stopped, shown as
//                     "Kapitel 4 · 42 %" instead of the page; "" = none
//   volumeNumber      ebooks of a combined work: the current volume, shown as
//                     "Band 2 · …"; 0 = a single book
//   hasOtherFormat    adds "auch als E-Book" / "auch als Hörbuch"
//   available         the file exists; otherwise the button is disabled
// Signals:
//   clicked()            the card, e.g. to open the details
//   continueRequested()  the round button
Rectangle {
    id: root

    required property string title
    required property string kind
    required property url coverUrl
    required property bool coverIsSquare
    required property real progress
    property date lastOpened: new Date(NaN)
    property int chapterNumber: 0
    property int volumeNumber: 0
    property int remainingSeconds: 0
    property int page: 0
    property int pages: 0
    property string chapterTitle
    property bool hasOtherFormat: false
    property bool available: true

    signal clicked()
    signal continueRequested()

    readonly property bool isAudiobook: root.kind === "audiobook"
    readonly property color formatColor: root.isAudiobook ? Theme.listen : Theme.read
    readonly property color formatForeground: root.isAudiobook ? Theme.listenForeground : Theme.readForeground

    function durationText(seconds: real): string {
        if (seconds < 60)
            return qsTr("unter 1 Min.")
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.round((seconds % 3600) / 60)
        return hours > 0 ? qsTr("%1 Std. %2 Min.").arg(hours).arg(minutes)
                         : qsTr("%1 Min.").arg(minutes)
    }

    // "Kap. 42 · noch 10 Std. 27 Min." / "Kapitel 4 · 42 %" / "Seite 88 von 210"
    function positionText(): string {
        if (root.isAudiobook) {
            const parts = []
            if (root.chapterNumber > 0)
                parts.push(qsTr("Kap. %1").arg(root.chapterNumber))
            if (root.remainingSeconds > 0)
                parts.push(qsTr("noch %1").arg(root.durationText(root.remainingSeconds)))
            return parts.length > 0 ? parts.join(" · ") : qsTr("Hörbuch")
        }
        // A volume of a combined work: "Band 2 · Kapitel 4 · 42 %".
        const volume = root.volumeNumber > 0 ? qsTr("Band %1").arg(root.volumeNumber) + " · " : ""
        if (root.chapterTitle !== "")
            return volume + qsTr("%1 · %2 %").arg(root.chapterTitle).arg(Math.round(root.progress * 100))
        if (root.page > 0 && root.pages > 0)
            return volume + qsTr("Seite %1 von %2").arg(root.page).arg(root.pages)
        return volume + (root.pages > 0 ? qsTr("%Ln Seiten", "", root.pages) : qsTr("E-Book"))
    }

    // "heute, 08:14", "gestern, 22:40", "So., 19:30" (last week), else the date.
    function dayText(date: date): string {
        const locale = Qt.locale(Qt.uiLanguage)
        const time = date.toLocaleTimeString(locale, Locale.ShortFormat)
        const today = new Date()
        today.setHours(0, 0, 0, 0)
        const day = new Date(date.getTime())
        day.setHours(0, 0, 0, 0)
        const days = Math.round((today.getTime() - day.getTime()) / 86400000)
        if (days <= 0)
            return qsTr("heute, %1").arg(time)
        if (days === 1)
            return qsTr("gestern, %1").arg(time)
        if (days < 7)
            return locale.dayName(date.getDay(), Locale.ShortFormat) + ", " + time
        return date.toLocaleDateString(locale, Locale.ShortFormat)
    }

    function lastOpenedText(): string {
        const parts = []
        if (!isNaN(root.lastOpened.getTime()))
            parts.push(qsTr("zuletzt: %1").arg(root.dayText(root.lastOpened)))
        if (root.hasOtherFormat)
            parts.push(root.isAudiobook ? qsTr("auch als E-Book") : qsTr("auch als Hörbuch"))
        return parts.join(" · ")
    }

    implicitWidth: Theme.continueCardWidth
    implicitHeight: cover.height + 2 * Theme.spacingM
    radius: Theme.radiusL
    color: cardHover.hovered ? Theme.surfaceRaised : Theme.surface
    border.width: Theme.borderWidth
    border.color: Theme.border

    Accessible.role: Accessible.Button
    Accessible.name: root.title + ", " + root.positionText()
    Accessible.onPressAction: root.clicked()

    Behavior on color {
        ColorAnimation { duration: Theme.durationFast }
    }

    CoverArt {
        id: cover

        anchors.left: parent.left
        anchors.leftMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.continueCoverWidth
        height: Math.round(width * Theme.coverAspect)
        title: root.title
        coverUrl: root.coverUrl
        coverIsSquare: root.coverIsSquare
    }

    Column {
        anchors.left: cover.right
        anchors.leftMargin: Theme.spacingM
        anchors.right: continueButton.left
        anchors.rightMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spacingXs

        Text {
            width: parent.width
            text: root.title
            elide: Text.ElideRight
            color: Theme.textPrimary
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
        }
        Item {
            width: parent.width
            height: position.implicitHeight

            Icon {
                id: kindIcon

                anchors.verticalCenter: parent.verticalCenter
                name: root.isAudiobook ? "headphones" : "book"
                size: Theme.iconSizeSmall + Theme.spacingXxs
                color: root.formatColor
            }
            Text {
                id: position

                anchors.left: kindIcon.right
                anchors.leftMargin: Theme.spacingXs
                anchors.right: parent.right
                text: root.positionText()
                elide: Text.ElideRight
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }
        ProgressLine {
            width: parent.width
            value: root.progress
            color: root.formatColor
        }
        Text {
            width: parent.width
            visible: text !== ""
            text: root.lastOpenedText()
            elide: Text.ElideRight
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
    }

    T.AbstractButton {
        id: continueButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.playButtonSize
        height: width
        enabled: root.available
        text: root.isAudiobook ? qsTr("Weiterhören") : qsTr("Weiterlesen")

        Accessible.name: continueButton.text

        onClicked: root.continueRequested()

        contentItem: Item {
            Icon {
                anchors.centerIn: parent
                // The play triangle looks centered slightly to the right.
                anchors.horizontalCenterOffset: root.isAudiobook ? Theme.spacingXxs : 0
                name: root.isAudiobook ? "play" : "book"
                color: root.formatForeground
            }
        }

        background: Rectangle {
            radius: width / 2
            opacity: continueButton.enabled ? 1 : Theme.disabledOpacity
            color: continueButton.down ? Qt.darker(root.formatColor, 1.15)
                                       : continueButton.hovered ? Qt.lighter(root.formatColor, 1.1)
                                                                : root.formatColor
            border.width: continueButton.visualFocus ? Theme.focusWidth : 0
            border.color: Theme.textPrimary
        }
    }

    HoverHandler {
        id: cardHover

        cursorShape: Qt.PointingHandCursor
    }
    TapHandler {
        // A click on the round button is not a click on the card.
        enabled: !continueButton.hovered
        onTapped: root.clicked()
    }
}

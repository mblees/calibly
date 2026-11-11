import QtQuick
import Calibly.Theme
import Calibly.Controls

// One work in the gallery: cover with format badges (top right) and progress
// lines (bottom: blue = read, orange = listened), title and author below.
// Presentation only, no logic.
//
// Properties:
//   title, author                 texts
//   coverUrl, coverIsSquare       cover image, see CoverArt
//   hasEbook, hasAudiobook        which format badges are shown
//   readProgress, listenProgress  0.0 – 1.0, 0 = no line
//   stacked                       draw the cover as a series stack
// Signals:
//   clicked()
Item {
    id: root

    required property string title
    required property string author
    required property url coverUrl
    required property bool coverIsSquare
    required property bool hasEbook
    required property bool hasAudiobook
    required property real readProgress
    required property real listenProgress
    property bool stacked: false

    readonly property bool hovered: hover.hovered

    signal clicked()

    implicitWidth: Theme.coverTileMinWidth
    implicitHeight: art.height + Theme.coverTileTextHeight

    Accessible.role: Accessible.Button
    Accessible.name: root.author !== "" ? root.title + ", " + root.author : root.title

    CoverArt {
        id: art

        width: parent.width
        height: Math.round(width * Theme.coverAspect)
        title: root.title
        author: root.author
        coverUrl: root.coverUrl
        coverIsSquare: root.coverIsSquare
        stacked: root.stacked

        Column {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.spacingS
            spacing: Theme.spacingXs

            Badge {
                visible: root.hasEbook
                iconName: "book"
                color: Theme.read
            }
            Badge {
                visible: root.hasAudiobook
                iconName: "headphones"
                color: Theme.listen
            }
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: Theme.borderWidth

            ProgressLine {
                width: parent.width
                visible: root.readProgress > 0
                value: root.readProgress
                color: Theme.read
                trackColor: Theme.coverProgressTrack
                rounded: false
            }
            ProgressLine {
                width: parent.width
                visible: root.listenProgress > 0
                value: root.listenProgress
                color: Theme.listen
                trackColor: Theme.coverProgressTrack
                rounded: false
            }
        }
    }

    // Hover outline around the cover (without the sheets of a stack).
    Rectangle {
        x: -Theme.spacingXxs
        y: art.stackInset - Theme.spacingXxs
        width: art.width - art.stackInset + 2 * Theme.spacingXxs
        height: art.height - art.stackInset + 2 * Theme.spacingXxs
        visible: root.hovered
        color: "transparent"
        radius: Theme.radiusS + Theme.spacingXxs
        border.width: Theme.focusWidth
        border.color: Theme.textPrimary
    }

    Text {
        id: titleText

        anchors.top: art.bottom
        anchors.topMargin: Theme.spacingS
        width: parent.width
        text: root.title
        elide: Text.ElideRight
        color: Theme.textPrimary
        font.pixelSize: Theme.fontBody
        font.weight: Font.DemiBold
    }

    Text {
        anchors.top: titleText.bottom
        width: parent.width
        text: root.author
        elide: Text.ElideRight
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
    }

    HoverHandler {
        id: hover

        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: root.clicked()
    }
}

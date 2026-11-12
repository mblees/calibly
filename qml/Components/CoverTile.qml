import QtQuick
import QtQuick.Effects
import Calibly.Theme
import Calibly.Controls
import Calibly.Models

// One work in the gallery: cover with format badges (top right), a state
// label (top left) and progress lines (bottom: blue = read, orange =
// listened, green = finished), title and author below. Hovering shows
// "Lesen" / "Hören" to continue directly. Presentation only, no logic.
//
// Properties:
//   title, author                 texts
//   coverUrl, coverIsSquare       cover image, see CoverArt
//   hasEbook, hasAudiobook        which format badges are shown
//   readProgress, listenProgress  0.0 – 1.0, 0 = no line
//   stacked                       draw the cover as a series stack
//   workState                     WorkState.New ("NEU"), WorkState.Finished
//                                 ("FERTIG"), WorkState.FileMissing (grey
//                                 cover, "DATEI FEHLT") or WorkState.Normal
//   dropLabel                     files are dragged over this cover: accent
//                                 glow and this text, e.g. "Als Hörbuch zu
//                                 „Dracula“ hinzufügen"; empty = no drag
// Signals:
//   clicked()
//   readRequested(), listenRequested()   the hover actions
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
    property int workState: WorkState.Normal
    property string dropLabel

    readonly property bool hovered: hover.hovered
    readonly property bool dropTarget: root.dropLabel !== ""
    readonly property bool fileMissing: root.workState === WorkState.FileMissing
    // Not while files are dragged over the cover, and not for missing files.
    readonly property bool showActions: root.hovered && !root.dropTarget && !root.fileMissing
                                        && (root.hasEbook || root.hasAudiobook)

    signal clicked()
    signal readRequested()
    signal listenRequested()

    implicitWidth: Theme.coverTileMinWidth
    implicitHeight: art.height + Theme.coverTileTextHeight

    Accessible.role: Accessible.Button
    Accessible.name: root.author !== "" ? root.title + ", " + root.author : root.title
    Accessible.onPressAction: root.clicked()

    CoverArt {
        id: art

        width: parent.width
        height: Math.round(width * Theme.coverAspect)
        title: root.title
        author: root.author
        coverUrl: root.coverUrl
        coverIsSquare: root.coverIsSquare
        stacked: root.stacked
        // The title of a generated cover starts below the state label.
        titleInset: root.workState !== WorkState.Normal ? Theme.badgeSize + Theme.spacingXs : 0

        // A missing file: the whole cover turns grey and darker.
        layer.enabled: root.fileMissing
        layer.effect: MultiEffect {
            saturation: Theme.missingSaturation
            brightness: Theme.missingBrightness
        }

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
                color: root.readProgress >= 1 ? Theme.finished : Theme.read
                trackColor: Theme.coverProgressTrack
                rounded: false
            }
            ProgressLine {
                width: parent.width
                visible: root.listenProgress > 0
                value: root.listenProgress
                color: root.listenProgress >= 1 ? Theme.finished : Theme.listen
                trackColor: Theme.coverProgressTrack
                rounded: false
            }
        }
    }

    // Hovered: the cover gets darker behind the actions "Lesen" / "Hören".
    Rectangle {
        y: art.stackInset
        width: art.width - art.stackInset
        height: art.height - art.stackInset
        visible: root.showActions
        radius: Theme.radiusS
        color: Theme.coverHoverScrim
    }

    // State label at the top left. Outside the cover, so the label of a
    // missing file keeps its color.
    Badge {
        x: Theme.spacingS
        y: art.stackInset + Theme.spacingS
        visible: root.workState !== WorkState.Normal
        text: root.workState === WorkState.New ? qsTr("NEU")
              : root.workState === WorkState.Finished ? qsTr("✓ FERTIG") : qsTr("DATEI FEHLT")
        color: root.workState === WorkState.New ? Theme.accentForeground
               : root.workState === WorkState.Finished ? Theme.finishedForeground : Theme.dangerForeground
        backgroundColor: root.workState === WorkState.New ? Theme.accent
                         : root.workState === WorkState.Finished ? Theme.finished : Theme.danger
    }

    // Hover actions: continue directly. Both are also in the detail view.
    Column {
        x: (art.width - art.stackInset - width) / 2
        y: art.stackInset + (art.height - art.stackInset - height) / 2
        width: Math.min(Theme.coverActionWidth, art.width - art.stackInset - 2 * Theme.spacingS)
        visible: root.showActions
        spacing: Theme.spacingS

        AppButton {
            id: readButton

            width: parent.width
            visible: root.hasEbook
            leftPadding: Theme.spacingS
            rightPadding: Theme.spacingS
            variant: AppButton.Read
            iconName: "book"
            text: qsTr("Lesen")
            onClicked: root.readRequested()
        }
        AppButton {
            id: listenButton

            width: parent.width
            visible: root.hasAudiobook
            leftPadding: Theme.spacingS
            rightPadding: Theme.spacingS
            variant: AppButton.Primary
            iconName: "play"
            text: qsTr("Hören")
            onClicked: root.listenRequested()
        }
    }

    // Hover outline around the cover (without the sheets of a stack).
    Rectangle {
        x: -Theme.spacingXxs
        y: art.stackInset - Theme.spacingXxs
        width: art.width - art.stackInset + 2 * Theme.spacingXxs
        height: art.height - art.stackInset + 2 * Theme.spacingXxs
        visible: root.hovered && !root.dropTarget
        color: "transparent"
        radius: Theme.radiusS + Theme.spacingXxs
        border.width: Theme.focusWidth
        border.color: Theme.textPrimary
    }

    // Drop target: accent outline with a soft glow and what will happen.
    Rectangle {
        x: -Theme.dropGlowWidth
        y: art.stackInset - Theme.dropGlowWidth
        width: art.width - art.stackInset + 2 * Theme.dropGlowWidth
        height: art.height - art.stackInset + 2 * Theme.dropGlowWidth
        visible: root.dropTarget
        color: "transparent"
        radius: Theme.radiusS + Theme.dropGlowWidth
        border.width: Theme.dropGlowWidth - Theme.focusWidth
        border.color: Qt.alpha(Theme.accent, Theme.dropGlowOpacity)

        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.dropGlowWidth - Theme.focusWidth
            color: "transparent"
            radius: Theme.radiusS + Theme.focusWidth
            border.width: Theme.focusWidth
            border.color: Theme.accent
        }
    }
    Rectangle {
        anchors.horizontalCenter: art.horizontalCenter
        y: art.stackInset + (art.height - art.stackInset - height) / 2
        width: Math.min(dropText.implicitWidth + 2 * Theme.spacingM, art.width - 2 * Theme.spacingS)
        height: dropText.implicitHeight + 2 * Theme.spacingS
        visible: root.dropTarget
        radius: Theme.radiusM
        color: Theme.accent

        Text {
            id: dropText

            anchors.fill: parent
            anchors.margins: Theme.spacingS
            text: root.dropLabel
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Theme.accentForeground
            font.pixelSize: Theme.fontSmall
            font.weight: Font.Bold
        }
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
        // A click on a hover action is not a click on the cover.
        enabled: !readButton.hovered && !listenButton.hovered
        onTapped: root.clicked()
    }
}

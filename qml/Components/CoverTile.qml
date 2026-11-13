// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Effects
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls
import Calibly.Models

// One work in the gallery: cover with format badges (top right), a state
// label (top left) and progress lines (bottom: blue = read, orange =
// listened, green = finished), title and author below. Hovering shows
// "Lesen" / "Hören" to continue directly, and a circle (bottom left) that
// selects the work; a selected work has an accent frame and a check mark.
// A series stack shows its volumes ("Band 1–4") instead of a state.
// Presentation only, no logic.
//
// Properties:
//   title, author                 texts
//   coverUrl, coverIsSquare       cover image, see CoverArt
//   hasEbook, hasAudiobook        which format badges are shown
//   readProgress, listenProgress  0.0 – 1.0, 0 = no line
//   stacked                       draw the cover as a series stack
//   stackLabel                    label of a stack, e.g. "Band 1–4"; shown
//                                 instead of the state label
//   selectable                    the work (or stack, for all its volumes) can be selected
//   selected                      it is selected
//   selecting                     other works are selected: the circle is
//                                 shown without hovering
//   workState                     WorkState.New ("NEU"), WorkState.Finished
//                                 ("FERTIG"), WorkState.FileMissing (grey
//                                 cover, "DATEI FEHLT") or WorkState.Normal
//   orderNumber                   a number at the bottom right, e.g. the
//                                 volume a click gave it when the order of
//                                 a series is chosen; 0 = none
//   dropLabel                     files are dragged over this cover: accent
//                                 glow and this text, e.g. "Als Hörbuch zu
//                                 „Dracula“ hinzufügen"; empty = no drag
// Signals:
//   clicked()
//   menuRequested(real x, real y)        right click, at this position of
//                                        the tile: the context menu
//   selectToggled()                      the circle, or Ctrl + click
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
    property string stackLabel
    property bool selectable: true
    property bool selected: false
    property bool selecting: false
    property int workState: WorkState.Normal
    property int orderNumber: 0
    property string dropLabel

    readonly property bool hovered: hover.hovered
    readonly property bool dropTarget: root.dropLabel !== ""
    readonly property bool fileMissing: root.workState === WorkState.FileMissing
    // Not while files are dragged over the cover, not for missing files,
    // not for a stack (it stands for several works) and not while titles are
    // selected (a click there is meant for the selection).
    readonly property bool showActions: root.hovered && !root.dropTarget && !root.fileMissing && !root.stacked
                                        && !root.selecting && !root.selected
                                        && (root.hasEbook || root.hasAudiobook)
    readonly property bool showSelectCircle: root.selectable && !root.dropTarget
                                             && (root.selected || root.selecting || root.hovered)
    readonly property bool hasLabel: root.stackLabel !== "" || root.workState !== WorkState.Normal

    signal clicked()
    signal menuRequested(real x, real y)
    signal selectToggled()
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
        // The title of a generated cover starts below the label.
        titleInset: root.hasLabel ? Theme.badgeSize + Theme.spacingXs : 0
        // The author of a generated cover makes room for the selection circle.
        authorInset: root.showSelectCircle ? Theme.selectCircleSize + Theme.spacingXs : 0

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

    // Label at the top left: the volumes of a stack, or the state. Outside
    // the cover, so the label of a missing file keeps its color.
    Badge {
        x: Theme.spacingS
        y: art.stackInset + Theme.spacingS
        visible: root.hasLabel
        text: root.stackLabel !== "" ? root.stackLabel
              : root.workState === WorkState.New ? qsTr("NEU")
              : root.workState === WorkState.Finished ? qsTr("✓ FERTIG") : qsTr("DATEI FEHLT")
        color: root.stackLabel !== "" ? Theme.textPrimary
               : root.workState === WorkState.New ? Theme.accentForeground
               : root.workState === WorkState.Finished ? Theme.finishedForeground : Theme.dangerForeground
        backgroundColor: root.stackLabel !== "" ? Theme.coverOverlay
                         : root.workState === WorkState.New ? Theme.accent
                         : root.workState === WorkState.Finished ? Theme.finished : Theme.danger
    }

    // Selection circle at the bottom left, above the progress lines: a ring
    // while hovering or selecting, filled with a check mark when selected.
    T.AbstractButton {
        id: selectButton

        x: Theme.spacingS
        y: art.height - height - Theme.spacingS - Theme.progressHeight
        width: Theme.selectCircleSize
        height: width
        visible: root.showSelectCircle
        text: root.selected ? qsTr("Auswahl aufheben") : qsTr("Auswählen")

        Accessible.name: selectButton.text

        onClicked: root.selectToggled()

        contentItem: Item {
            Icon {
                anchors.centerIn: parent
                visible: root.selected
                name: "check"
                size: Theme.iconSizeSmall + Theme.spacingXxs
                strokeWidth: Theme.badgeIconStrokeWidth
                color: Theme.accentForeground
            }
        }

        background: Rectangle {
            radius: width / 2
            color: root.selected ? Theme.accent : Theme.coverOverlay
            border.width: root.selected ? 0 : Theme.focusWidth
            border.color: selectButton.hovered ? Theme.accent : Theme.textPrimary
        }
    }

    // Number at the bottom right, above the progress lines.
    Rectangle {
        x: art.width - width - Theme.spacingS
        y: art.height - height - Theme.spacingS - Theme.progressHeight
        width: Math.max(Theme.orderBadgeSize, orderText.implicitWidth + 2 * Theme.spacingS)
        height: Theme.orderBadgeSize
        visible: root.orderNumber > 0
        radius: height / 2
        color: Theme.accent

        Accessible.role: Accessible.StaticText
        Accessible.name: qsTr("Band %1").arg(root.orderNumber)

        Text {
            id: orderText

            anchors.centerIn: parent
            text: root.orderNumber
            color: Theme.accentForeground
            font.pixelSize: Theme.fontLarge
            font.weight: Font.Bold
        }
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

    // Hover outline around the cover (without the sheets of a stack); a
    // selected cover has a thicker one, outside the cover as well.
    Rectangle {
        id: outline

        readonly property int frameWidth: root.selected ? Theme.selectionWidth : Theme.focusWidth

        x: -outline.frameWidth
        y: art.stackInset - outline.frameWidth
        width: art.width - art.stackInset + 2 * outline.frameWidth
        height: art.height - art.stackInset + 2 * outline.frameWidth
        visible: (root.hovered || root.selected) && !root.dropTarget
        color: "transparent"
        radius: Theme.radiusS + outline.frameWidth
        border.width: outline.frameWidth
        // Selected: accent frame, also without hovering.
        border.color: root.selected ? Theme.accent : Theme.textPrimary
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

    // A click on a hover action or the circle is not a click on the cover.
    TapHandler {
        acceptedModifiers: Qt.NoModifier
        enabled: !readButton.hovered && !listenButton.hovered && !selectButton.hovered
        onTapped: root.clicked()
    }
    // Right click: the context menu.
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: function(eventPoint) {
            root.menuRequested(eventPoint.position.x, eventPoint.position.y)
        }
    }
    // Ctrl + click selects.
    TapHandler {
        acceptedModifiers: Qt.ControlModifier
        enabled: root.selectable && !readButton.hovered && !listenButton.hovered && !selectButton.hovered
        onTapped: root.selectToggled()
    }
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Theme
import Calibly.Controls

// Player bar at the bottom of the window: cover, title, transport buttons,
// position in the current chapter, speed, sleep timer and the button that
// opens the fullscreen player. All times in milliseconds.
//
// Properties:
//   title, coverUrl   the audiobook
//   subtitle         e.g. "Kap. 42 · Die Weiße des Wals · Andreas Lindner"
//   errorText        shown instead of the subtitle, "" = none
//   playing
//   chapterPosition, chapterDuration   position within the current chapter
//   chapterNumber, chapterCount        e.g. 42 / 135
//   speed, sleepMinutesLeft, sleepAtChapterEnd   see PlaybackOptions
// Signals:
//   playToggled(), skipRequested(int seconds), previousRequested(), nextRequested(),
//   seekRequested(real chapterPosition), speedPicked(real), sleepPicked(int),
//   expandRequested(), closeRequested()
Rectangle {
    id: root

    property string title
    property url coverUrl
    property string subtitle
    property string errorText
    property bool playing: false
    property real chapterPosition: 0
    property real chapterDuration: 0
    property int chapterNumber: 0
    property int chapterCount: 0
    property real speed: 1.0
    property int sleepMinutesLeft: 0
    property bool sleepAtChapterEnd: false

    signal playToggled()
    signal skipRequested(int seconds)
    signal previousRequested()
    signal nextRequested()
    signal seekRequested(real chapterPosition)
    signal speedPicked(real speed)
    signal sleepPicked(int minutes)
    signal expandRequested()
    signal closeRequested()

    // While dragging, the times follow the mouse.
    readonly property real shownPosition: seekBar.pressed ? seekBar.dragValue : root.chapterPosition

    implicitWidth: Theme.drawerWidth * 2
    implicitHeight: Theme.playerBarHeight
    color: Theme.playerBar

    Rectangle {
        width: parent.width
        height: Theme.borderWidth
        color: Theme.border
    }

    // A click on cover or title opens the fullscreen player.
    MouseArea {
        id: infoArea

        x: Theme.spacingL
        width: info.x + info.width - x
        height: parent.height
        cursorShape: Qt.PointingHandCursor
        onClicked: root.expandRequested()
    }

    CoverArt {
        id: cover

        x: Theme.spacingL
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.playerBarCoverSize
        height: width
        title: root.title
        coverUrl: root.coverUrl
        coverIsSquare: false // the square frame already has the shape of audiobook covers
    }

    Column {
        id: info

        anchors.left: cover.right
        anchors.leftMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.playerBarInfoWidth
        spacing: Theme.spacingXxs

        Text {
            width: parent.width
            elide: Text.ElideRight
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontBody
            font.weight: Font.Bold
        }
        Text {
            width: parent.width
            elide: Text.ElideRight
            text: root.errorText !== "" ? root.errorText : root.subtitle
            color: root.errorText !== "" ? Theme.danger : Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
    }

    PlayerControls {
        id: controls

        anchors.left: info.right
        anchors.leftMargin: Theme.spacingL
        anchors.verticalCenter: parent.verticalCenter
        playing: root.playing
        onPreviousRequested: root.previousRequested()
        onSkipBackRequested: root.skipRequested(-30)
        onPlayToggled: root.playToggled()
        onSkipForwardRequested: root.skipRequested(30)
        onNextRequested: root.nextRequested()
    }

    Text {
        id: elapsed

        anchors.left: controls.right
        anchors.leftMargin: Theme.spacingL
        anchors.verticalCenter: parent.verticalCenter
        text: TimeFormat.clock(root.shownPosition)
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        font.features: { "tnum": 1 }
    }

    SeekBar {
        id: seekBar

        anchors.left: elapsed.right
        anchors.right: remaining.left
        anchors.leftMargin: Theme.spacingS
        anchors.rightMargin: Theme.spacingS
        anchors.verticalCenter: parent.verticalCenter
        to: root.chapterDuration
        value: root.chapterPosition
        Accessible.name: qsTr("Position im Kapitel")
        onSeekRequested: function(value) {
            root.seekRequested(value)
        }
    }

    Text {
        id: remaining

        anchors.right: chapter.visible ? chapter.left : options.left
        anchors.rightMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        text: TimeFormat.remaining(root.chapterDuration - root.shownPosition)
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        font.features: { "tnum": 1 }
    }

    Text {
        id: chapter

        anchors.right: options.left
        anchors.rightMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        visible: root.chapterCount > 1
        text: qsTr("Kap. %1 / %2").arg(root.chapterNumber).arg(root.chapterCount)
        color: Theme.textDisabled
        font.pixelSize: Theme.fontSmall
    }

    PlaybackOptions {
        id: options

        anchors.right: buttons.left
        anchors.rightMargin: Theme.spacingS
        anchors.verticalCenter: parent.verticalCenter
        popupAbove: true
        speed: root.speed
        sleepMinutesLeft: root.sleepMinutesLeft
        sleepAtChapterEnd: root.sleepAtChapterEnd
        onSpeedPicked: function(speed) {
            root.speedPicked(speed)
        }
        onSleepPicked: function(minutes) {
            root.sleepPicked(minutes)
        }
    }

    Row {
        id: buttons

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter

        IconButton {
            iconName: "expand"
            text: qsTr("Player öffnen")
            onClicked: root.expandRequested()
        }
        IconButton {
            iconName: "x"
            text: qsTr("Player schließen")
            onClicked: root.closeRequested()
        }
    }
}

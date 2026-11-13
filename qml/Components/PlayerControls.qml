// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// Transport buttons of the player: previous chapter, 30 s back,
// play / pause, 30 s forward, next chapter.
//
// Properties:
//   playing  shows pause instead of play
//   large    big buttons for the fullscreen player
// Signals:
//   previousRequested(), skipBackRequested(), playToggled(),
//   skipForwardRequested(), nextRequested()
Row {
    id: root

    property bool playing: false
    property bool large: false

    signal previousRequested()
    signal skipBackRequested()
    signal playToggled()
    signal skipForwardRequested()
    signal nextRequested()

    readonly property real iconSize: root.large ? Theme.iconSizePlayer : Theme.iconSize

    spacing: root.large ? Theme.spacingXl : Theme.spacingXs

    component TransportButton: IconButton {
        anchors.verticalCenter: parent.verticalCenter
        width: root.large ? Theme.controlHeight + Theme.spacingM : Theme.controlHeight
        height: width
        iconSize: root.iconSize
        iconColor: Theme.textPrimary
    }

    TransportButton {
        iconName: "previous"
        text: qsTr("Voriges Kapitel")
        onClicked: root.previousRequested()
    }
    TransportButton {
        iconName: "skipBack"
        text: qsTr("30 Sekunden zurück")
        onClicked: root.skipBackRequested()
    }

    T.AbstractButton {
        id: playButton

        anchors.verticalCenter: parent.verticalCenter
        width: root.large ? Theme.playButtonSizeLarge : Theme.playButtonSize
        height: width
        text: root.playing ? qsTr("Pause") : qsTr("Abspielen")

        Accessible.name: playButton.text

        onClicked: root.playToggled()

        contentItem: Item {
            Icon {
                anchors.centerIn: parent
                // The play triangle looks centered slightly to the right.
                anchors.horizontalCenterOffset: root.playing ? 0 : Theme.spacingXxs
                name: root.playing ? "pause" : "play"
                size: root.large ? Theme.iconSizePlayer + Theme.spacingXs : Theme.iconSize
                color: Theme.playButtonForeground
            }
        }

        background: Rectangle {
            radius: width / 2
            color: playButton.down ? Qt.darker(Theme.playButton, 1.15)
                                   : playButton.hovered ? Qt.darker(Theme.playButton, 1.05) : Theme.playButton
            border.width: playButton.visualFocus ? Theme.focusWidth : 0
            border.color: Theme.accent
        }
    }

    TransportButton {
        iconName: "skipForward"
        text: qsTr("30 Sekunden vor")
        onClicked: root.skipForwardRequested()
    }
    TransportButton {
        iconName: "next"
        text: qsTr("Nächstes Kapitel")
        onClicked: root.nextRequested()
    }
}

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// View C of the mockup: the player in fullscreen, opened from the player bar.
// Large cover over a blurred copy of it, the position within the current
// chapter (the overall progress small below), transport buttons, speed,
// sleep timer and the chapter list. Space plays / pauses, ← / → skip
// 30 seconds; Esc or "Zurück zur Bibliothek" closes it.
T.Popup {
    id: root

    readonly property real chapterPosition: PlayerController.position - PlayerController.chapterStart
    readonly property real shownPosition: seekBar.pressed ? seekBar.dragValue : root.chapterPosition

    parent: T.Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    padding: 0
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape

    // Nothing to show any more, e.g. after the player was closed.
    Connections {
        target: PlayerController

        function onWorkChanged() {
            if (!PlayerController.hasWork)
                root.close()
        }
    }

    WorkDetails {
        id: details

        model: LibraryController.works
        workId: PlayerController.workId
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationDrawer }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    background: Rectangle {
        color: Theme.background
        clip: true

        // Background from the colors of the cover: decoded tiny and scaled
        // up, so only the colors remain, then blurred.
        Image {
            anchors.fill: parent
            anchors.margins: -Theme.coverBlurMax * 2
            visible: status === Image.Ready
            source: details.audiobookCoverUrl
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            smooth: true
            sourceSize.width: Theme.playerBlurSourceSize
            sourceSize.height: Theme.playerBlurSourceSize
            layer.enabled: true
            layer.effect: MultiEffect {
                autoPaddingEnabled: false
                blurEnabled: true
                blur: 1.0
                blurMax: Theme.coverBlurMax
                brightness: Theme.playerBlurBrightness
                saturation: Theme.playerBlurSaturation
            }
        }
    }

    T.Overlay.modal: Rectangle {
        color: Theme.background
    }

    contentItem: Item {
        focus: true

        Keys.onSpacePressed: PlayerController.togglePlay()
        Keys.onLeftPressed: PlayerController.skip(-30)
        Keys.onRightPressed: PlayerController.skip(30)

        // --- top bar -----------------------------------------------------------
        T.AbstractButton {
            id: back

            x: Theme.spacingL
            y: Theme.spacingM
            height: Theme.controlHeight
            width: backRow.implicitWidth + 2 * Theme.spacingS
            text: qsTr("Zurück zur Bibliothek")

            Accessible.name: back.text

            onClicked: root.close()

            contentItem: Item {
                Row {
                    id: backRow

                    anchors.centerIn: parent
                    spacing: Theme.spacingS

                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        name: "chevronDown"
                        color: Theme.textPrimary
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: back.text
                        color: back.hovered ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                }
            }
            background: Rectangle {
                radius: Theme.radiusM
                color: back.hovered ? Theme.hoverTint : "transparent"
                border.width: back.visualFocus ? Theme.focusWidth : 0
                border.color: Theme.textPrimary
            }
        }

        // --- left: cover, position, buttons ------------------------------------
        Column {
            id: main

            readonly property real sideWidth: Math.min(Theme.playerColumnWidth, parent.width * 0.5)

            x: (sideWidth - width) / 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: Theme.spacingL
            width: sideWidth - 2 * Theme.spacingXxl
            spacing: Theme.spacingL

            CoverArt {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(Theme.playerCoverSize, root.height * 0.38)
                height: details.audiobookCoverIsSquare ? width : Math.round(width * Theme.coverAspect)
                title: details.title
                author: details.authors.join(", ")
                coverUrl: details.audiobookCoverUrl
                coverIsSquare: false // the frame already has the cover's shape
            }

            Column {
                width: parent.width
                spacing: Theme.spacingXs

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: {
                        const title = PlayerController.chapters[PlayerController.chapterIndex]?.title ?? ""
                        if (PlayerController.chapters.length <= 1)
                            return title !== "" ? title : details.title
                        return qsTr("Kap. %1 · %2").arg(PlayerController.chapterIndex + 1)
                                   .arg(title !== "" ? title : qsTr("Kapitel %1").arg(PlayerController.chapterIndex + 1))
                    }
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Bold
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: {
                        const parts = [details.title]
                        if (details.authors.length > 0)
                            parts.push(details.authors.join(", "))
                        if (details.narrators.length > 0)
                            parts.push(qsTr("gelesen von %1").arg(details.narrators.join(", ")))
                        return parts.join(" · ")
                    }
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: PlayerController.error !== PlayerController.NoError
                    text: qsTr("Die Datei konnte nicht abgespielt werden.")
                    color: Theme.danger
                    font.pixelSize: Theme.fontBody
                }
            }

            Column {
                width: parent.width
                spacing: Theme.spacingXs

                SeekBar {
                    id: seekBar

                    width: parent.width
                    large: true
                    to: PlayerController.chapterDuration
                    value: root.chapterPosition
                    Accessible.name: qsTr("Position im Kapitel")
                    onSeekRequested: function(value) {
                        PlayerController.seek(PlayerController.chapterStart + value)
                    }
                }
                Item {
                    width: parent.width
                    height: elapsed.implicitHeight

                    Text {
                        id: elapsed

                        text: TimeFormat.clock(root.shownPosition)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        font.features: { "tnum": 1 }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: PlayerController.chapters.length > 1
                        text: qsTr("Kapitel %1 von %2").arg(PlayerController.chapterIndex + 1)
                                                          .arg(PlayerController.chapters.length)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                    }
                    Text {
                        anchors.right: parent.right
                        text: TimeFormat.remaining(PlayerController.chapterDuration - root.shownPosition)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        font.features: { "tnum": 1 }
                    }
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Gesamt: %1 von %2 · %3 %")
                              .arg(TimeFormat.clock(PlayerController.position, true))
                              .arg(TimeFormat.clock(PlayerController.duration, true))
                              .arg(PlayerController.duration > 0
                                   ? Math.floor(100 * PlayerController.position / PlayerController.duration) : 0)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.features: { "tnum": 1 }
                }
            }

            PlayerControls {
                anchors.horizontalCenter: parent.horizontalCenter
                large: true
                playing: PlayerController.playing
                onPreviousRequested: PlayerController.previousChapter()
                onSkipBackRequested: PlayerController.skip(-30)
                onPlayToggled: PlayerController.togglePlay()
                onSkipForwardRequested: PlayerController.skip(30)
                onNextRequested: PlayerController.nextChapter()
            }

            PlaybackOptions {
                anchors.horizontalCenter: parent.horizontalCenter
                speed: PlayerController.speed
                sleepMinutesLeft: PlayerController.sleepMinutesLeft
                sleepAtChapterEnd: PlayerController.sleepAtChapterEnd
                onSpeedPicked: function(speed) {
                    PlayerController.speed = speed
                }
                onSleepPicked: function(minutes) {
                    if (minutes < 0)
                        PlayerController.setSleepAtChapterEnd()
                    else
                        PlayerController.setSleepTimer(minutes)
                }
            }
        }

        // --- right: chapters ---------------------------------------------------
        Rectangle {
            id: panel

            anchors.left: parent.left
            anchors.leftMargin: main.sideWidth
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: Theme.spacingXxl + Theme.controlHeight
            anchors.rightMargin: Theme.spacingXl
            anchors.bottomMargin: Theme.spacingXl
            radius: Theme.radiusL
            color: Theme.playerPanel

            Tabs {
                id: tabs

                x: Theme.spacingL
                y: Theme.spacingM
                width: parent.width - 2 * Theme.spacingL
                tabs: [qsTr("Kapitel · %1").arg(PlayerController.chapters.length)]
                currentIndex: 0
            }

            ChapterList {
                anchors.left: tabs.left
                anchors.right: parent.right
                anchors.top: tabs.bottom
                anchors.bottom: parent.bottom
                anchors.topMargin: Theme.spacingS
                anchors.bottomMargin: Theme.spacingM
                anchors.rightMargin: Theme.spacingS
                chapters: PlayerController.chapters
                currentIndex: PlayerController.chapterIndex
                currentProgress: PlayerController.chapterDuration > 0
                                 ? root.chapterPosition / PlayerController.chapterDuration : 0
                onChapterActivated: function(index) {
                    PlayerController.playChapter(index)
                }
            }
        }
    }
}

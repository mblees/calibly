import QtQuick
import Calibly.Theme
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// The player bar at the bottom of the window (shown while an audiobook is
// loaded) and the fullscreen player it opens. Connects MiniPlayer and
// PlayerView to PlayerController.
//
// Functions:
//   openPlayer()  shows the fullscreen player
Item {
    id: root

    function openPlayer() {
        if (PlayerController.hasWork)
            playerView.open()
    }

    implicitHeight: PlayerController.hasWork ? Theme.playerBarHeight : 0
    visible: PlayerController.hasWork

    WorkDetails {
        id: details

        model: LibraryController.works
        workId: PlayerController.workId
    }

    MiniPlayer {
        anchors.fill: parent
        title: details.title
        coverUrl: details.audiobookCoverUrl
        subtitle: {
            const parts = []
            if (PlayerController.chapters.length > 1) {
                const title = PlayerController.chapters[PlayerController.chapterIndex]?.title ?? ""
                parts.push(title !== "" ? qsTr("Kap. %1 · %2").arg(PlayerController.chapterIndex + 1).arg(title)
                                        : qsTr("Kapitel %1").arg(PlayerController.chapterIndex + 1))
            }
            if (details.narrators.length > 0)
                parts.push(details.narrators.join(", "))
            else if (details.authors.length > 0)
                parts.push(details.authors.join(", "))
            return parts.join(" · ")
        }
        errorText: PlayerController.error !== PlayerController.NoError
                   ? qsTr("Die Datei konnte nicht abgespielt werden.") : ""
        playing: PlayerController.playing
        chapterPosition: PlayerController.position - PlayerController.chapterStart
        chapterDuration: PlayerController.chapterDuration
        chapterNumber: PlayerController.chapterIndex + 1
        chapterCount: PlayerController.chapters.length
        speed: PlayerController.speed
        sleepMinutesLeft: PlayerController.sleepMinutesLeft
        sleepAtChapterEnd: PlayerController.sleepAtChapterEnd

        onPlayToggled: PlayerController.togglePlay()
        onSkipRequested: function(seconds) {
            PlayerController.skip(seconds)
        }
        onPreviousRequested: PlayerController.previousChapter()
        onNextRequested: PlayerController.nextChapter()
        onSeekRequested: function(chapterPosition) {
            PlayerController.seek(PlayerController.chapterStart + chapterPosition)
        }
        onSpeedPicked: function(speed) {
            PlayerController.speed = speed
        }
        onSleepPicked: function(minutes) {
            if (minutes < 0)
                PlayerController.setSleepAtChapterEnd()
            else
                PlayerController.setSleepTimer(minutes)
        }
        onExpandRequested: playerView.open()
        onCloseRequested: PlayerController.close()
    }

    PlayerView {
        id: playerView
    }
}

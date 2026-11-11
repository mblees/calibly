import QtQuick
import Calibly.Theme
import Calibly.Controls

// Playback speed and sleep timer of the player, as two small menus.
//
// Properties:
//   speed              current playback rate, e.g. 1.25
//   sleepMinutesLeft   minutes until the sleep timer pauses, 0 = off
//   sleepAtChapterEnd  the sleep timer pauses at the end of the chapter
//   popupAbove         open the menus upwards (bar at the bottom)
// Signals:
//   speedPicked(real speed)
//   sleepPicked(int minutes)   0 = off, -1 = at the end of the chapter
Row {
    id: root

    property real speed: 1.0
    property int sleepMinutesLeft: 0
    property bool sleepAtChapterEnd: false
    property bool popupAbove: false

    signal speedPicked(real speed)
    signal sleepPicked(int minutes)

    readonly property var speeds: [0.75, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0, 2.5]
    readonly property var sleepMinutes: [0, 15, 30, 45, 60, -1]

    function speedText(value: real): string {
        return qsTr("%1×").arg(value.toLocaleString(Qt.locale(Qt.uiLanguage), "f", value * 100 % 10 === 0 ? 1 : 2))
    }

    spacing: Theme.spacingS

    SelectMenu {
        popupAbove: root.popupAbove
        options: root.speeds.map(value => root.speedText(value))
        currentIndex: root.speeds.indexOf(root.speed)
        valueText: root.speedText(root.speed)
        Accessible.name: qsTr("Tempo: %1").arg(root.speedText(root.speed))
        onActivated: function(index) {
            root.speedPicked(root.speeds[index])
        }
    }

    SelectMenu {
        popupAbove: root.popupAbove
        iconName: "moon"
        options: [qsTr("Kein Schlaftimer"), qsTr("15 Min."), qsTr("30 Min."), qsTr("45 Min."), qsTr("60 Min."),
                  qsTr("Ende des Kapitels")]
        currentIndex: root.sleepAtChapterEnd ? 5 : 0
        valueText: root.sleepAtChapterEnd ? qsTr("Kapitelende")
                 : root.sleepMinutesLeft > 0 ? qsTr("%1 Min.").arg(root.sleepMinutesLeft)
                 : qsTr("Aus")
        Accessible.name: qsTr("Schlaftimer: %1").arg(valueText)
        onActivated: function(index) {
            root.sleepPicked(root.sleepMinutes[index])
        }
    }
}

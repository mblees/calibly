import QtQuick
import Calibly.Theme

// Thin progress bar without text. The width is set by the user of the control.
//
// Properties:
//   value       progress 0.0 – 1.0
//   color       color of the filled part (e.g. Theme.read, Theme.listen)
//   trackColor  color of the empty part
//   rounded     rounded ends (default) or straight ends
Item {
    id: root

    property real value: 0
    property color color: Theme.accent
    property color trackColor: Theme.surfaceRaised
    property bool rounded: true

    implicitHeight: Theme.progressHeight

    Accessible.role: Accessible.ProgressBar

    Rectangle {
        anchors.fill: parent
        radius: root.rounded ? height / 2 : 0
        color: root.trackColor
    }

    Rectangle {
        width: parent.width * Math.max(0, Math.min(1, root.value))
        height: parent.height
        radius: root.rounded ? height / 2 : 0
        color: root.color
    }
}

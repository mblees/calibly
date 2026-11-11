import QtQuick
import Calibly.Theme

// Small marker: a round icon (e.g. the format on a cover) or a short text
// label (e.g. "Band 1–4"). If text is set, the badge becomes a pill with text.
//
// Properties:
//   iconName         icon, a key of Icons.paths
//   text             label text, shown instead of the icon
//   color            icon / text color
//   backgroundColor  background
Item {
    id: root

    property string iconName
    property string text
    property color color: Theme.textPrimary
    property color backgroundColor: Theme.coverOverlay

    readonly property bool hasText: root.text !== ""

    implicitWidth: root.hasText ? label.implicitWidth + 2 * Theme.spacingS : Theme.badgeSize
    implicitHeight: Theme.badgeSize

    Accessible.role: Accessible.StaticText
    Accessible.name: root.text

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.backgroundColor
    }

    Icon {
        anchors.centerIn: parent
        visible: !root.hasText
        name: root.iconName
        size: Theme.iconSizeSmall
        strokeWidth: Theme.badgeIconStrokeWidth
        color: root.color
    }

    Text {
        id: label

        anchors.centerIn: parent
        visible: root.hasText
        text: root.text
        color: root.color
        font.pixelSize: Theme.fontSmall
        font.weight: Font.Bold
    }
}

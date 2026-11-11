import QtQuick
import Calibly.Theme

// Stand-in for views that are not built yet.
ViewBase {
    id: root

    Column {
        anchors.centerIn: parent
        spacing: Theme.spacingM

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.iconName
            size: Theme.iconSizeLarge
            strokeWidth: 1.5
            color: Theme.textDisabled
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Diese Ansicht ist noch ein Platzhalter.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
        }
    }
}

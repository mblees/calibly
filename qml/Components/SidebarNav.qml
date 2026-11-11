pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Calibly.Theme

// Left navigation bar with the app name and one entry per view.
//
// Properties:
//   model         list model with the roles
//                   title    (string) label
//                   iconName (string) key of Icons.paths
//                   count    (int)    number shown on the right, 0 = hidden
//   currentIndex  index of the selected entry
// Signals:
//   activated(int index)  the user picked an entry
Rectangle {
    id: root

    property alias model: entries.model
    property int currentIndex: 0

    signal activated(int index)

    implicitWidth: Theme.sidebarWidth
    color: Theme.sidebar

    Rectangle {
        anchors.right: parent.right
        width: Theme.borderWidth
        height: parent.height
        color: Theme.divider
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingS
        anchors.topMargin: Theme.spacingL
        spacing: Theme.spacingL

        Row {
            Layout.leftMargin: Theme.spacingS

            Text {
                text: "cali"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
            }
            Text {
                text: "bly"
                color: Theme.accent
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
            }
        }

        ListView {
            id: entries

            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingXxs
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: T.ItemDelegate {
                id: entry

                required property int index
                required property string title
                required property string iconName
                required property int count

                width: ListView.view.width
                implicitHeight: Theme.controlHeight
                leftPadding: Theme.spacingS + Theme.spacingXxs
                rightPadding: Theme.spacingS + Theme.spacingXxs
                highlighted: entry.index === root.currentIndex
                text: entry.title

                Accessible.name: entry.title

                onClicked: root.activated(entry.index)

                contentItem: RowLayout {
                    spacing: Theme.spacingM

                    Icon {
                        name: entry.iconName
                        color: entry.highlighted ? Theme.accent : Theme.textSecondary
                    }
                    Text {
                        Layout.fillWidth: true
                        text: entry.title
                        elide: Text.ElideRight
                        color: entry.highlighted ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                    Text {
                        visible: entry.count > 0
                        // Locale-aware number (1.284 / 1,284) that follows the UI language.
                        text: Number(entry.count).toLocaleString(Qt.locale(Qt.uiLanguage), "f", 0)
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSmall
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusM
                    color: entry.highlighted ? Theme.surfaceSelected
                                             : entry.hovered ? Theme.surface : "transparent"
                    border.width: entry.visualFocus ? Theme.focusWidth : 0
                    border.color: Theme.textPrimary

                    Behavior on color {
                        ColorAnimation { duration: Theme.durationFast }
                    }
                }
            }
        }
    }
}

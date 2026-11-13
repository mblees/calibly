pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Calibly.Theme

// Left navigation bar with the app name and one entry per view, and
// entries at the bottom (e.g. "Einstellungen").
//
// Properties:
//   model         list model with the roles
//                   title    (string) label
//                   iconName (string) key of Icons.paths
//                   count    (int)    number shown on the right, 0 = hidden
//   bottomModel   entries at the bottom, same roles; their indices follow
//                 those of model (model.count, model.count + 1, ...)
//   currentIndex  index of the selected entry
// Signals:
//   activated(int index)  the user picked an entry
Rectangle {
    id: root

    property alias model: entries.model
    property alias bottomModel: bottomEntries.model
    property int currentIndex: 0

    signal activated(int index)

    // One entry; offset: added to its index (entries at the bottom).
    component Entry: T.ItemDelegate {
        id: entry

        required property int index
        property int offset: 0
        readonly property int entryIndex: entry.index + entry.offset
        required property string title
        required property string iconName
        required property int count

        implicitHeight: Theme.controlHeight
        leftPadding: Theme.spacingS + Theme.spacingXxs
        rightPadding: Theme.spacingS + Theme.spacingXxs
        highlighted: entry.entryIndex === root.currentIndex
        text: entry.title

        Accessible.name: entry.title

        onClicked: root.activated(entry.entryIndex)

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

        // Wordmark: one color and weight, the color is in the icon
        // (mockups/calibly-logo-ideen.html, idea 4).
        Text {
            Layout.leftMargin: Theme.spacingS
            text: "calibly"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }

        ListView {
            id: entries

            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingXxs
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Entry {
                width: ListView.view.width
            }
        }

        Column {
            Layout.fillWidth: true
            spacing: Theme.spacingXxs

            Repeater {
                id: bottomEntries

                delegate: Entry {
                    width: parent ? parent.width : 0
                    offset: entries.count
                }
            }
        }
    }
}

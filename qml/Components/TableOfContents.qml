pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme

// The contents of a book in the reader: a column with the heading "INHALT"
// and one row per entry with its page. The entry of the shown page is
// marked; sub-entries are indented.
//
// Properties:
//   entries       [{ title, level (0 = top), page (0-based, -1 = not known yet) }]
//   currentIndex  the entry of the shown page, -1 = none
// Signals:
//   entryActivated(int index)  the user picked an entry
Rectangle {
    id: root

    property var entries: []
    property int currentIndex: -1

    signal entryActivated(int index)

    implicitWidth: Theme.contentsWidth
    implicitHeight: Theme.popupMaxHeight
    color: Theme.sidebar

    Rectangle {
        width: Theme.borderWidth
        height: parent.height
        color: Theme.border
    }

    Text {
        id: heading

        x: Theme.spacingL + Theme.spacingS
        y: Theme.spacingL
        text: qsTr("Inhalt")
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 1
    }

    Text {
        x: heading.x
        anchors.top: heading.bottom
        anchors.topMargin: Theme.spacingM
        width: parent.width - 2 * heading.x
        visible: root.entries.length === 0
        text: qsTr("Dieses Buch hat kein Inhaltsverzeichnis.")
        wrapMode: Text.Wrap
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }

    ListView {
        id: list

        anchors.fill: parent
        anchors.leftMargin: Theme.spacingM
        anchors.rightMargin: Theme.spacingS
        anchors.topMargin: heading.y + heading.height + Theme.spacingS
        anchors.bottomMargin: Theme.spacingM
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        spacing: Theme.spacingXxs
        model: root.entries.length
        currentIndex: root.currentIndex
        highlightFollowsCurrentItem: false

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Inhalt")

        ScrollBar.vertical: ScrollBar {}

        // Keep the current entry in view.
        onCurrentIndexChanged: {
            if (list.currentIndex >= 0)
                list.positionViewAtIndex(list.currentIndex, ListView.Contain)
        }

        delegate: T.ItemDelegate {
            id: row

            required property int index

            readonly property var entry: root.entries[row.index] ?? ({})
            readonly property bool current: row.index === root.currentIndex

            width: ListView.view ? ListView.view.width - Theme.spacingS : 0
            implicitHeight: Theme.contentsRowHeight
            leftPadding: Theme.spacingS + Math.min(row.entry.level ?? 0, 3) * Theme.spacingM
            rightPadding: Theme.spacingS
            text: row.entry.title ?? ""

            Accessible.name: row.text

            onClicked: root.entryActivated(row.index)

            background: Rectangle {
                radius: Theme.radiusS + Theme.spacingXxs
                color: row.current ? Qt.alpha(Theme.read, Theme.currentRowOpacity)
                                   : row.hovered ? Theme.surface : "transparent"
                border.width: row.visualFocus ? Theme.focusWidth : 0
                border.color: Theme.textPrimary
            }

            contentItem: Item {
                Text {
                    anchors.left: parent.left
                    anchors.right: pageNumber.left
                    anchors.rightMargin: Theme.spacingS
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: row.text
                    color: row.current ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                    font.weight: row.current ? Font.DemiBold : Font.Normal
                }
                Text {
                    id: pageNumber

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: (row.entry.page ?? -1) >= 0 ? String(row.entry.page + 1) : ""
                    color: row.current ? Theme.read : Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.features: { "tnum": 1 }
                }
            }
        }
    }
}

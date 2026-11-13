pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// Bar at the bottom of the library while works are selected (view A): how
// many, and the few things that can be done with them. Presentation only.
//
// Properties:
//   count     number of selected works
//   canMerge  "Zu einem Werk verbinden" is possible with this selection
//   busy      the actions that change the library are disabled, e.g. while
//             an import is running
//   exporting an export is running: "Exportieren …" is disabled
// Signals:
//   metadataRequested()  edit the metadata of the selected works
//   mergeRequested()     join them into one work
//   exportRequested()    copy their files into a folder
//   removeRequested()    remove them from the library
//   closeRequested()     ✕: end the selection
Rectangle {
    id: root

    property int count: 0
    property bool canMerge: false
    property bool busy: false
    property bool exporting: false

    signal metadataRequested()
    signal mergeRequested()
    signal exportRequested()
    signal removeRequested()
    signal closeRequested()

    // Flat button of the bar: icon and text.
    component BarButton: T.AbstractButton {
        id: button

        property string iconName
        property color color: Theme.textPrimary

        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: buttonRow.implicitWidth + 2 * Theme.spacingM
        implicitHeight: Theme.controlHeight
        opacity: button.enabled ? 1 : Theme.disabledOpacity

        Accessible.name: button.text

        contentItem: Item {
            Row {
                id: buttonRow

                anchors.centerIn: parent
                spacing: Theme.spacingS

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: button.iconName
                    size: Theme.iconSize - Theme.spacingXxs
                    color: button.color
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: button.text
                    color: button.color
                    font.pixelSize: Theme.fontBody
                }
            }
        }

        background: Rectangle {
            radius: Theme.radiusM
            color: button.down || button.hovered ? Theme.hoverTint : "transparent"
            border.width: button.visualFocus ? Theme.focusWidth : 0
            border.color: Theme.textPrimary
        }
    }

    implicitWidth: content.implicitWidth + 2 * Theme.spacingS
    implicitHeight: Theme.controlHeight + 2 * Theme.spacingS
    radius: Theme.radiusL
    color: Theme.surfaceRaised
    border.width: Theme.borderWidth
    border.color: Theme.inputBorder

    Accessible.role: Accessible.ToolBar
    Accessible.name: countText.text

    Row {
        id: content

        anchors.centerIn: parent
        spacing: Theme.spacingXs

        Text {
            id: countText

            anchors.verticalCenter: parent.verticalCenter
            leftPadding: Theme.spacingM
            rightPadding: Theme.spacingM
            text: qsTr("%Ln ausgewählt", "", root.count)
            color: Theme.textPrimary
            font.pixelSize: Theme.fontBody
            font.weight: Font.Bold
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.borderWidth
            height: Theme.iconSize
            color: Theme.inputBorder
        }
        BarButton {
            iconName: "edit"
            text: qsTr("Metadaten")
            onClicked: root.metadataRequested()
        }
        BarButton {
            iconName: "link"
            text: qsTr("Zu einem Werk verbinden")
            enabled: root.canMerge && !root.busy
            onClicked: root.mergeRequested()
        }
        BarButton {
            iconName: "upload"
            text: qsTr("Exportieren …")
            enabled: !root.exporting
            onClicked: root.exportRequested()
        }
        BarButton {
            iconName: "trash"
            text: qsTr("Entfernen")
            color: Theme.danger
            enabled: !root.busy
            onClicked: root.removeRequested()
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "x"
            iconSize: Theme.iconSizeSmall + Theme.spacingXxs
            text: qsTr("Auswahl aufheben")
            onClicked: root.closeRequested()
        }
    }
}

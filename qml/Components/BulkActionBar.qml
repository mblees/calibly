// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// Bar at the bottom of the library while works are selected (view A): how
// many, and the things that can be done with them. Presentation only.
//
// Properties:
//   count     number of selected works
//   canMerge  "Kombinieren" is possible with this selection
//   busy      the actions that change the library are disabled, e.g. while
//             an import is running
//   exporting an export is running: "Exportieren …" is disabled
// Signals:
//   metadataRequested()  edit the metadata (only with one selected work)
//   mergeRequested()     "Kombinieren": join them into one entry
//   seriesRequested()    "Als Serie …": give them one series
//   exportRequested()    copy their files into a folder
//   finishedRequested()  "⋯" → mark all as finished
//   newRequested()       "⋯" → mark all as new
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
    signal seriesRequested()
    signal exportRequested()
    signal finishedRequested()
    signal newRequested()
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
                    visible: button.text !== ""
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
        // Metadata are edited for one title at a time.
        BarButton {
            iconName: "edit"
            text: qsTr("Metadaten")
            enabled: root.count === 1
            onClicked: root.metadataRequested()
        }
        BarButton {
            iconName: "link"
            text: qsTr("Kombinieren")
            enabled: root.canMerge && !root.busy
            onClicked: root.mergeRequested()
        }
        BarButton {
            iconName: "layers"
            text: qsTr("Als Serie …")
            enabled: root.count > 1 && !root.busy
            onClicked: root.seriesRequested()
        }
        BarButton {
            iconName: "upload"
            text: qsTr("Exportieren …")
            enabled: !root.exporting
            onClicked: root.exportRequested()
        }
        BarButton {
            id: moreButton

            iconName: "more"
            enabled: !root.busy
            Accessible.name: qsTr("Weitere Aktionen")
            onClicked: moreMenu.popup(moreButton, 0, -moreMenu.implicitHeight - Theme.spacingXs)
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

    AppMenu {
        id: moreMenu

        AppMenuItem {
            iconName: "check"
            text: qsTr("Alle als fertig markieren")
            onTriggered: root.finishedRequested()
        }
        AppMenuItem {
            iconName: "reset"
            text: qsTr("Alle als neu markieren …")
            onTriggered: root.newRequested()
        }
    }
}

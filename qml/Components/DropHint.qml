// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import Calibly.Theme

// Hint at the top of the gallery while files are dragged over it (view D):
// shows what happens on release. The active case is highlighted.
// Presentation only, no logic.
//
// Properties:
//   onCover   the pointer is over a cover: the files become formats of that
//             work; otherwise (free space) they become new works
//   summary   what is dragged, e.g. "Dracula_Hoerbuch · 27 MP3" (optional)
Rectangle {
    id: root

    property bool onCover: false
    property string summary

    // One of the two cases: icon, bold place, normal result.
    component Case: Rectangle {
        id: entry

        required property string iconName
        required property string place
        required property string result
        required property bool active

        implicitWidth: caseRow.implicitWidth + 2 * Theme.spacingM
        implicitHeight: Theme.chipHeight - Theme.spacingXs
        radius: height / 2
        color: entry.active ? Qt.alpha(Theme.accent, Theme.currentRowOpacity) : "transparent"

        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }

        Row {
            id: caseRow

            anchors.centerIn: parent
            spacing: Theme.spacingS

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: entry.iconName
                size: Theme.iconSizeSmall + Theme.spacingXxs
                color: entry.active ? Theme.accent : Theme.textMuted
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: entry.place
                color: entry.active ? Theme.textPrimary : Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.Bold
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: entry.result
                color: entry.active ? Theme.textSecondary : Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }
    }

    implicitWidth: content.implicitWidth + 2 * Theme.spacingXs
    implicitHeight: Theme.chipHeight + Theme.spacingXs
    radius: height / 2
    color: Theme.surfaceRaised
    border.width: Theme.borderWidth
    border.color: Theme.border

    Accessible.role: Accessible.StaticText
    Accessible.name: root.onCover ? qsTr("Auf ein Cover = Format ergänzen")
                                  : qsTr("Freie Fläche = als neuer Titel")

    Row {
        id: content

        anchors.centerIn: parent
        spacing: Theme.spacingXs

        Case {
            iconName: "plus"
            place: qsTr("Freie Fläche")
            result: qsTr("= als neuer Titel")
            active: !root.onCover
        }
        Case {
            iconName: "link"
            place: qsTr("Auf ein Cover")
            result: qsTr("= Format ergänzen")
            active: root.onCover
        }
        Row {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.summary !== ""
            spacing: Theme.spacingS
            rightPadding: Theme.spacingM

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.borderWidth
                height: Theme.iconSize
                color: Theme.border
            }
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "folder"
                size: Theme.iconSizeSmall + Theme.spacingXxs
                color: Theme.textMuted
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.summary
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }
    }
}

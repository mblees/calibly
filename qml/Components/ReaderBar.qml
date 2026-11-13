// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// The bar at the top of the reader: "‹ Bibliothek", title, author and
// chapter, and the buttons for the contents, the text settings ("Aa") and
// fullscreen. In fullscreen it lies over the page, with the title in the
// middle. Presentation only.
//
// Properties:
//   backText          label of the way back, e.g. "Bibliothek"
//   title, author
//   chapterTitle      the chapter of the shown page, "" = none
//   pdf               the "Aa" button shows a magnifier: PDFs only zoom
//   fullscreen        the look in fullscreen and the button to leave it
//   contentsShown     the contents button is shown as switched on
//   settingsOpen      the "Aa" button is shown as switched on
//   settingsButton    the "Aa" button, to open the text settings at it (read-only)
// Signals:
//   backRequested(), contentsToggled(), settingsToggled(), fullscreenToggled()
Rectangle {
    id: root

    property string backText: qsTr("Bibliothek")
    property string title
    property string author
    property string chapterTitle
    property bool pdf: false
    property bool fullscreen: false
    property bool contentsShown: false
    property bool settingsOpen: false
    readonly property Item settingsButton: aaButton

    signal backRequested()
    signal contentsToggled()
    signal settingsToggled()
    signal fullscreenToggled()

    readonly property string subtitle: [root.author, root.chapterTitle].filter(part => part !== "").join(" · ")

    implicitWidth: Theme.drawerWidth * 2
    implicitHeight: Theme.readerBarHeight
    color: root.fullscreen ? Theme.readerBar : Theme.background

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.borderWidth
        color: Theme.border
    }

    T.AbstractButton {
        id: back

        anchors.left: parent.left
        anchors.leftMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        width: backRow.implicitWidth + 2 * Theme.spacingS + Theme.spacingXxs
        height: Theme.controlHeight
        text: root.backText

        Accessible.name: qsTr("Zurück: %1").arg(root.backText)

        onClicked: root.backRequested()

        contentItem: Item {
            Row {
                id: backRow

                anchors.centerIn: parent
                spacing: Theme.spacingXs

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevronLeft"
                    color: back.hovered ? Theme.textPrimary : Theme.textSecondary
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: back.text
                    color: back.hovered ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }
            }
        }
        background: Rectangle {
            radius: Theme.radiusM
            color: back.hovered ? Theme.hoverTint : "transparent"
            border.width: back.visualFocus ? Theme.focusWidth : 0
            border.color: Theme.textPrimary
        }
    }

    // Title and author · chapter: next to "‹ Bibliothek" in the window,
    // centered in fullscreen.
    Item {
        id: titleArea

        readonly property real centeredWidth: root.width - 2 * Math.max(back.width + back.anchors.leftMargin,
                                                                        buttons.width + buttons.anchors.rightMargin)
                                              - 2 * Theme.spacingM

        x: root.fullscreen ? (root.width - width) / 2 : back.x + back.width + Theme.spacingS
        width: root.fullscreen ? centeredWidth : buttons.x - Theme.spacingM - x
        height: parent.height

        Row {
            visible: !root.fullscreen
            width: parent.width
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spacingS

            Text {
                id: windowTitle

                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, parent.width)
                elide: Text.ElideRight
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: Theme.fontLarge - 1
                font.weight: Font.DemiBold
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, parent.width - windowTitle.width - parent.spacing)
                elide: Text.ElideRight
                text: root.subtitle
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
            }
        }

        Column {
            visible: root.fullscreen
            width: parent.width
            anchors.verticalCenter: parent.verticalCenter

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: Theme.fontLarge - 1
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                visible: text !== ""
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: root.subtitle
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall + 1
            }
        }
    }

    Row {
        id: buttons

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingM
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spacingS

        IconButton {
            round: false
            iconName: "list"
            text: root.contentsShown ? qsTr("Inhalt ausblenden") : qsTr("Inhalt einblenden")
            checked: root.contentsShown
            onClicked: root.contentsToggled()
        }

        // "Aa": the text settings, for PDFs the zoom.
        IconButton {
            id: aaButton

            round: false
            iconName: root.pdf ? "search" : ""
            text: root.pdf ? qsTr("Ansicht") : qsTr("Texteinstellungen")
            checked: root.settingsOpen
            onClicked: root.settingsToggled()

            Text {
                anchors.centerIn: parent
                visible: !root.pdf
                text: "Aa"
                color: aaButton.hovered || aaButton.checked ? Theme.textPrimary : Theme.textSecondary
                font.family: Theme.fontFamilySerif
                font.pixelSize: Theme.fontLarge + 1
            }
        }

        IconButton {
            round: false
            iconName: root.fullscreen ? "shrink" : "expand"
            text: root.fullscreen ? qsTr("Vollbild beenden") : qsTr("Vollbild")
            onClicked: root.fullscreenToggled()
        }
    }
}

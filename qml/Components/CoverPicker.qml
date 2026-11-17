// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import Calibly.Theme
import Calibly.Controls

// The cover in the metadata editor (phase 14): the cover in its frame, a
// picture file can be dropped on it, and "Cover ändern" below opens a menu
// with the ways to change it. Presentation only: the editor does the work.
//
// Properties:
//   title, author    for the placeholder
//   coverUrl         the cover, empty = placeholder
//   coverIsSquare    the frame is square (audiobook cover)
//   shared           the change applies to e-book and audiobook (🔗 next to the button)
//   modified         the cover was changed and is not saved yet: said below the button
//   canPaste         the clipboard holds a picture (asked when the menu opens)
//   otherText        label of "take the other format's cover", "" = not offered
//   canReadFile      the book / audiobook file can give its cover back
//   hasCover         "Cover entfernen" is offered
//   dropActive       a picture is dragged over the cover (read-only)
// Signals:
//   menuAboutToOpen()   update canPaste now
//   pickRequested(), pasteRequested(), otherRequested(), fileRequested(),
//   removeRequested()
//   imageDropped(url fileUrl)
Item {
    id: root

    property string title
    property string author
    property url coverUrl
    property bool coverIsSquare: false
    property bool shared: false
    property bool modified: false
    property bool canPaste: false
    property string otherText
    property bool canReadFile: false
    property bool hasCover: root.coverUrl.toString() !== ""
    readonly property bool dropActive: drop.containsDrag

    signal menuAboutToOpen()
    signal pickRequested()
    signal pasteRequested()
    signal otherRequested()
    signal fileRequested()
    signal removeRequested()
    signal imageDropped(url fileUrl)

    // Picture files the cover takes, by their suffix.
    function isImage(url: url): bool {
        return /\.(jpe?g|png|webp|bmp|gif|tiff?)$/i.test(url.toString())
    }

    activeFocusOnTab: true
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("Cover")

    implicitWidth: Theme.editorCoverWidth
    implicitHeight: frame.height + Theme.spacingM + changeButton.height + Theme.spacingXs + hint.implicitHeight

    CoverArt {
        id: frame

        width: parent.width
        height: root.coverIsSquare ? width : Math.round(width * Theme.coverAspect)
        title: root.title
        author: root.author
        coverUrl: root.coverUrl
        coverIsSquare: false // the frame already has the cover's shape

        // A picture dragged over the cover: accent glow as in the gallery.
        Rectangle {
            anchors.fill: parent
            visible: root.dropActive
            color: Qt.alpha(Theme.accent, Theme.dropGlowOpacity)
            border.width: Theme.dropGlowWidth
            border.color: Theme.accent

            Text {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacingM
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("Als Cover verwenden")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
        }
    }

    // A click on the cover gives it the focus, so Ctrl+V pastes there and
    // not into a text field.
    TapHandler {
        target: frame
        onTapped: root.forceActiveFocus()
    }
    Rectangle {
        anchors.fill: frame
        anchors.margins: -Theme.focusWidth
        visible: root.activeFocus
        color: "transparent"
        radius: Theme.radiusM
        border.width: Theme.focusWidth
        border.color: Theme.textPrimary
    }

    DropArea {
        id: drop

        anchors.fill: frame
        keys: ["text/uri-list"]

        onEntered: function(event) {
            event.accepted = event.urls.length === 1 && root.isImage(event.urls[0])
        }
        onDropped: function(event) {
            if (event.urls.length === 1 && root.isImage(event.urls[0])) {
                root.imageDropped(event.urls[0])
                event.acceptProposedAction()
            }
        }
    }

    AppButton {
        id: changeButton

        anchors.top: frame.bottom
        anchors.topMargin: Theme.spacingM
        width: parent.width
        iconName: "image"
        text: qsTr("Cover ändern")
        onClicked: {
            root.menuAboutToOpen()
            menu.popup(changeButton, 0, changeButton.height + Theme.spacingXs)
        }

        Icon {
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingS
            anchors.verticalCenter: parent.verticalCenter
            visible: root.shared
            name: "link"
            size: Theme.iconSizeSmall
            color: Theme.shared
        }
    }

    Text {
        id: hint

        anchors.top: changeButton.bottom
        anchors.topMargin: Theme.spacingXs
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        // Not saved yet: said here instead of on the cover.
        text: root.modified ? qsTr("Geändert · wird mit „Speichern“ übernommen")
                            : qsTr("oder ein Bild auf das Cover ziehen")
        color: root.modified ? Theme.accent : Theme.textMuted
        font.pixelSize: Theme.fontSmall
    }

    AppMenu {
        id: menu

        AppMenuItem {
            iconName: "folder"
            text: qsTr("Bild auswählen …")
            onTriggered: root.pickRequested()
        }
        AppMenuItem {
            iconName: "clipboard"
            enabled: root.canPaste
            text: qsTr("Bild einfügen (Strg+V)")
            onTriggered: root.pasteRequested()
        }
        AppMenuItem {
            visible: root.otherText !== ""
            iconName: "layers"
            text: root.otherText
            onTriggered: root.otherRequested()
        }
        AppMenuItem {
            visible: root.canReadFile
            iconName: "reset"
            text: qsTr("Cover aus der Datei")
            onTriggered: root.fileRequested()
        }
        AppMenuSeparator {
            visible: root.hasCover
        }
        AppMenuItem {
            visible: root.hasCover
            iconName: "trash"
            danger: true
            text: qsTr("Cover entfernen")
            onTriggered: root.removeRequested()
        }
    }
}

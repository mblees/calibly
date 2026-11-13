// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// "Als Serie zusammenfassen …" for titles without (usable) volume numbers,
// see LibraryController.seriesNeedsOrder(): only these titles, as in the
// library, and the user clicks them in the order of the series. Each one
// shows the number it gets; a second click takes it out again, the later
// ones move up. "Überspringen" numbers them as before (setSeries()), without
// asking. Opens with openFor(); Esc, ✕ and "Abbrechen" change nothing.
T.Popup {
    id: root

    // The titles, in the order they are shown.
    property list<string> workIds: []
    property string seriesName
    // The format selection of the library: the covers of that format.
    property int formatFilter: WorkFilterModel.AllFormats
    // The titles that were clicked, in that order.
    property list<string> order: []

    // The volume number each position gets.
    readonly property var numbers: LibraryController.seriesNumbers(root.workIds, root.seriesName)
    readonly property bool complete: root.workIds.length > 0 && root.order.length === root.workIds.length

    function openFor(workIds: list<string>, seriesName: string, formatFilter: int) {
        root.workIds = workIds
        root.seriesName = seriesName
        root.formatFilter = formatFilter
        root.order = []
        root.open()
    }

    // A click on a title: next number, or out of the order again.
    function toggle(workId: string) {
        const changed = root.order.slice()
        const index = changed.indexOf(workId)
        if (index >= 0)
            changed.splice(index, 1)
        else
            changed.push(workId)
        root.order = changed
    }

    function save() {
        if (!root.complete)
            return
        LibraryController.setSeriesOrder(root.order, root.seriesName)
        root.close()
    }

    function skip() {
        LibraryController.setSeries(root.workIds, root.seriesName)
        root.close()
    }

    parent: T.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(Theme.dialogWidth, parent ? parent.width - 2 * Theme.spacingXl : Theme.dialogWidth)
    height: Math.min(Theme.dialogHeight, parent ? parent.height - 2 * Theme.spacingXl : Theme.dialogHeight)
    padding: 0
    modal: true
    focus: true
    // No closing by a click outside: the clicked order would be lost too easily.
    closePolicy: T.Popup.CloseOnEscape

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    background: Rectangle {
        radius: Theme.radiusL
        color: Theme.dialog
        border.width: Theme.borderWidth
        border.color: Theme.inputBorder
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
    }

    WorkFilterModel {
        id: titles

        sourceModel: LibraryController.works
        formatFilter: root.formatFilter
        onlyIds: root.workIds
    }

    contentItem: Item {
        // --- header ----------------------------------------------------------
        Item {
            id: header

            width: parent.width
            height: Theme.controlHeight + 2 * Theme.spacingM

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingL
                anchors.right: closeButton.left
                anchors.rightMargin: Theme.spacingL
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingS

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "layers"
                    color: Theme.textSecondary
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, parent.width - Theme.iconSize - parent.spacing)
                    elide: Text.ElideRight
                    text: qsTr("Reihenfolge der Serie „%1“").arg(root.seriesName)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                }
            }

            IconButton {
                id: closeButton

                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingM
                anchors.verticalCenter: parent.verticalCenter
                iconName: "x"
                iconColor: Theme.textPrimary
                text: qsTr("Schließen")
                onClicked: root.close()
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: Theme.borderWidth
                color: Theme.border
            }
        }

        Text {
            id: hint

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: header.bottom
            anchors.margins: Theme.spacingXl
            wrapMode: Text.WordWrap
            lineHeight: Theme.lineHeightText
            text: qsTr("Die Titel haben keine durchgehenden Bandnummern. Die Bände in der Reihenfolge der Serie anklicken; ein zweiter Klick nimmt einen Band wieder heraus.")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }

        // --- the titles, as in the library -----------------------------------
        GridView {
            id: grid

            readonly property int gap: Theme.spacingL
            readonly property int columns: Math.max(2, Math.floor((width + gap) / (Theme.coverTileMinWidth + gap)))
            readonly property real tileWidth: cellWidth - gap

            anchors.top: hint.bottom
            anchors.bottom: footerBar.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: Theme.spacingL
            anchors.leftMargin: Theme.spacingXl
            // The gap of the last column is part of the cell.
            anchors.rightMargin: Theme.spacingXl - gap

            cellWidth: Math.floor(width / columns)
            cellHeight: Math.round(tileWidth * Theme.coverAspect) + Theme.coverTileTextHeight + Theme.spacingL
            // Room for the frame of a numbered cover.
            topMargin: Theme.selectionWidth
            bottomMargin: Theme.spacingL
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: titles

            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                id: cell

                required property string workId
                required property string title
                required property string authorText
                required property url coverUrl
                required property bool coverIsSquare
                required property bool hasEbook
                required property bool hasAudiobook
                required property real readProgress
                required property real listenProgress
                required property int workState

                readonly property int position: root.order.indexOf(cell.workId)

                width: grid.cellWidth
                height: grid.cellHeight

                CoverTile {
                    x: Theme.selectionWidth
                    width: grid.tileWidth - Theme.selectionWidth
                    title: cell.title
                    author: cell.authorText
                    coverUrl: cell.coverUrl
                    coverIsSquare: cell.coverIsSquare
                    hasEbook: cell.hasEbook
                    hasAudiobook: cell.hasAudiobook
                    readProgress: cell.readProgress
                    listenProgress: cell.listenProgress
                    workState: cell.workState
                    // A click numbers the title: no circle, no "Lesen" / "Hören".
                    selectable: false
                    selecting: true
                    selected: cell.position >= 0
                    orderNumber: cell.position >= 0 && cell.position < root.numbers.length
                                 ? root.numbers[cell.position] : 0

                    onClicked: root.toggle(cell.workId)
                    onSelectToggled: root.toggle(cell.workId)
                }
            }
        }

        // --- footer ----------------------------------------------------------
        Item {
            id: footerBar

            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.controlHeight + 2 * Theme.spacingM

            Rectangle {
                width: parent.width
                height: Theme.borderWidth
                color: Theme.border
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingL
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingL

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("%1 von %2 Bänden festgelegt").arg(root.order.length).arg(root.workIds.length)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
                LinkButton {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.order.length > 0
                    text: qsTr("Zurücksetzen")
                    onClicked: root.order = []
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingL
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingS

                AppButton {
                    text: qsTr("Abbrechen")
                    onClicked: root.close()
                }
                // As without asking: numbers the titles have stay, the others
                // get the free ones in the shown order.
                AppButton {
                    text: qsTr("Überspringen")
                    onClicked: root.skip()
                }
                AppButton {
                    text: qsTr("Speichern")
                    variant: AppButton.Primary
                    enabled: root.complete
                    onClicked: root.save()
                }
            }
        }
    }
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// Search in the book (phase 14): a column in the reader in place of the
// contents, with the search field, the number of results, ‹ › to go through
// them and the list of results with the words around them. Enter in the
// field goes to the next result, Shift + Enter to the previous one; Esc
// empties the field, a second Esc closes the search. Presentation only.
//
// Properties:
//   text           the search text
//   results        [{ title, before, match, after }]
//   currentIndex   the current result, -1 = none
//   searching      still searching (more results may come)
//   limited        stopped at the maximum number of results
// Functions:
//   focusField()   puts the cursor into the field and selects its text
// Signals:
//   textEdited(string text)
//   resultActivated(int index), nextRequested(), previousRequested()
//   closeRequested()
Rectangle {
    id: root

    property string text
    property var results: []
    property int currentIndex: -1
    property bool searching: false
    property bool limited: false

    signal textEdited(string text)
    signal resultActivated(int index)
    signal nextRequested()
    signal previousRequested()
    signal closeRequested()

    function focusField() {
        field.forceActiveFocus()
        field.selectAll()
    }

    function escaped(text: string): string {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    readonly property string statusText: {
        if (root.text.trim() === "")
            return qsTr("Wörter oder Sätze im Buch finden")
        const count = root.results.length
        if (count === 0)
            return root.searching ? qsTr("Sucht …") : qsTr("Nichts gefunden")
        if (root.limited)
            return qsTr("Die ersten %Ln Treffer (es gibt mehr)", "", count)
        if (root.currentIndex >= 0)
            return qsTr("%1 von %2").arg(root.currentIndex + 1).arg(count) + (root.searching ? " …" : "")
        return qsTr("%Ln Treffer", "", count) + (root.searching ? " …" : "")
    }

    implicitWidth: Theme.searchPanelWidth
    implicitHeight: Theme.popupMaxHeight
    color: Theme.sidebar

    // Esc in an empty field ends up here.
    Keys.onEscapePressed: root.closeRequested()

    Rectangle {
        width: Theme.borderWidth
        height: parent.height
        color: Theme.border
    }

    Text {
        id: heading

        x: Theme.spacingL + Theme.spacingS
        anchors.verticalCenter: closeButton.verticalCenter
        text: qsTr("Suchen")
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 1
    }

    IconButton {
        id: closeButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingS
        y: Theme.spacingS
        iconName: "x"
        text: qsTr("Suche schließen")
        onClicked: root.closeRequested()
    }

    SearchField {
        id: field

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.spacingL
        anchors.rightMargin: Theme.spacingL
        anchors.top: closeButton.bottom
        anchors.topMargin: Theme.spacingXs
        placeholderText: qsTr("Im Buch suchen")
        text: root.text
        onTextEdited: root.textEdited(field.text)

        Keys.onReturnPressed: function(event) {
            if (event.modifiers & Qt.ShiftModifier)
                root.previousRequested()
            else
                root.nextRequested()
        }
        Keys.onEnterPressed: function(event) {
            if (event.modifiers & Qt.ShiftModifier)
                root.previousRequested()
            else
                root.nextRequested()
        }
    }

    Item {
        id: status

        anchors.left: field.left
        anchors.right: field.right
        anchors.top: field.bottom
        anchors.topMargin: Theme.spacingS
        height: Theme.controlHeight

        Text {
            anchors.left: parent.left
            anchors.right: navigation.left
            anchors.rightMargin: Theme.spacingS
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            text: root.statusText
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }

        Row {
            id: navigation

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: root.results.length > 0

            IconButton {
                iconName: "chevronUp"
                text: qsTr("Vorheriger Treffer (Umschalt+F3)")
                onClicked: root.previousRequested()
            }
            IconButton {
                iconName: "chevronDown"
                text: qsTr("Nächster Treffer (F3)")
                onClicked: root.nextRequested()
            }
        }
    }

    ListView {
        id: list

        anchors.fill: parent
        anchors.leftMargin: Theme.spacingM
        anchors.rightMargin: Theme.spacingS
        anchors.topMargin: status.y + status.height + Theme.spacingXs
        anchors.bottomMargin: Theme.spacingM
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        spacing: Theme.spacingXxs
        model: root.results.length
        currentIndex: root.currentIndex
        highlightFollowsCurrentItem: false

        Accessible.role: Accessible.List
        Accessible.name: qsTr("Suchergebnisse")

        ScrollBar.vertical: ScrollBar {}

        // Keep the current result in view.
        onCurrentIndexChanged: {
            if (list.currentIndex >= 0)
                list.positionViewAtIndex(list.currentIndex, ListView.Contain)
        }

        delegate: T.ItemDelegate {
            id: row

            required property int index

            readonly property var result: root.results[row.index] ?? ({})
            readonly property bool current: row.index === root.currentIndex

            width: ListView.view ? ListView.view.width - Theme.spacingS : 0
            implicitHeight: content.implicitHeight + 2 * Theme.spacingS
            leftPadding: Theme.spacingS
            rightPadding: Theme.spacingS
            topPadding: Theme.spacingS
            bottomPadding: Theme.spacingS

            Accessible.name: (row.result.before ?? "") + (row.result.match ?? "") + (row.result.after ?? "")

            onClicked: root.resultActivated(row.index)

            background: Rectangle {
                radius: Theme.radiusS + Theme.spacingXxs
                color: row.current ? Qt.alpha(Theme.read, Theme.currentRowOpacity)
                                   : row.hovered ? Theme.surface : "transparent"
                border.width: row.visualFocus ? Theme.focusWidth : 0
                border.color: Theme.textPrimary
            }

            contentItem: Column {
                id: content

                spacing: Theme.spacingXxs

                Text {
                    width: parent.width
                    visible: text !== ""
                    elide: Text.ElideRight
                    text: row.result.title ?? ""
                    color: row.current ? Theme.read : Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
                    text: root.escaped(row.result.before ?? "") + "<b>" + root.escaped(row.result.match ?? "")
                          + "</b>" + root.escaped(row.result.after ?? "")
                    color: row.current ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                    lineHeight: Theme.lineHeightText
                }
            }
        }
    }
}

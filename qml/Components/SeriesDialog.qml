// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import Calibly.Theme
import Calibly.Controls

// "Als Serie zusammenfassen …": asks for the name of the series the
// selected titles get (phase 12). Presentation only.
//
// Properties:
//   count        number of selected titles
//   name         the name in the field; set it before open() as a suggestion
//   suggestions  names of existing series; those that match the typed text
//                are offered as chips (at most Theme.seriesSuggestionCount)
// Signals:
//   accepted()   "Zusammenfassen": the dialog is closed, name holds the name
ConfirmDialog {
    id: root

    property int count: 0
    property string name
    property var suggestions: []

    readonly property var matches: {
        const typed = root.name.trim().toLowerCase()
        const list = []
        for (const series of root.suggestions) {
            if (list.length >= Theme.seriesSuggestionCount)
                break
            if (series.toLowerCase() !== typed && (typed === "" || series.toLowerCase().includes(typed)))
                list.push(series)
        }
        return list
    }

    title: qsTr("%Ln Titel als Serie zusammenfassen", "", root.count)
    message: qsTr("Alle bekommen diesen Seriennamen. Fehlen Bandnummern oder kommen sie doppelt vor, wird danach die Reihenfolge der Bände festgelegt.")
    confirmText: qsTr("Zusammenfassen")
    confirmEnabled: root.name.trim() !== ""

    onOpened: {
        field.forceActiveFocus()
        field.selectAll()
    }

    InputField {
        id: field

        width: parent.width
        text: root.name
        placeholderText: qsTr("Name der Serie")
        onTextEdited: root.name = field.text
        Keys.onReturnPressed: root.confirm()
        Keys.onEnterPressed: root.confirm()
    }
    Flow {
        width: parent.width
        visible: root.matches.length > 0
        spacing: Theme.spacingS

        Repeater {
            model: root.matches

            delegate: Chip {
                required property string modelData

                text: modelData
                onClicked: root.name = modelData
            }
        }
    }
}

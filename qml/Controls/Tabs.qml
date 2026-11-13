// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Row of tabs with an accent line under the current one.
//
// Properties:
//   tabs          list of tab titles
//   currentIndex  index of the current tab
// Signals:
//   activated(int index)  the user picked a tab
Item {
    id: root

    property var tabs: []
    property int currentIndex: 0

    signal activated(int index)

    implicitWidth: row.implicitWidth
    implicitHeight: Theme.tabHeight

    Accessible.role: Accessible.PageTabList

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.borderWidth
        color: Theme.border
    }

    Row {
        id: row

        height: parent.height
        spacing: Theme.spacingXl

        Repeater {
            model: root.tabs

            delegate: T.AbstractButton {
                id: tab

                required property string modelData
                required property int index

                readonly property bool current: tab.index === root.currentIndex

                height: row.height
                implicitWidth: label.implicitWidth
                text: tab.modelData

                Accessible.role: Accessible.PageTab
                Accessible.name: tab.modelData

                onClicked: root.activated(tab.index)

                contentItem: Text {
                    id: label

                    text: tab.modelData
                    verticalAlignment: Text.AlignVCenter
                    color: tab.current || tab.hovered ? Theme.textPrimary : Theme.textMuted
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                }

                background: Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: Theme.focusWidth
                    visible: tab.current
                    color: Theme.accent
                }
            }
        }
    }
}

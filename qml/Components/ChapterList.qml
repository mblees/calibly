// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// List of the chapters of an audiobook. Chapters before the current one are
// marked as heard, the current one is highlighted and shows its progress.
//
// Properties:
//   chapters         [{ title ("" = untitled), start, duration }] in ms
//   currentIndex     current chapter, -1 = none (e.g. not playing this book)
//   currentProgress  0 – 1 within the current chapter
//   scrollable       false: as high as all rows (inside another scroll view)
// Signals:
//   chapterActivated(int index)  the user picked a chapter
ListView {
    id: root

    property var chapters: []
    property real currentProgress: 0
    property bool scrollable: true

    signal chapterActivated(int index)

    // "3. Die Weiße des Wals", or "Kapitel 3" for untitled chapters.
    function chapterTitle(index: int): string {
        const title = root.chapters[index]?.title ?? ""
        return title !== "" ? qsTr("%1. %2").arg(index + 1).arg(title) : qsTr("Kapitel %1").arg(index + 1)
    }

    implicitWidth: Theme.drawerWidth
    implicitHeight: root.scrollable ? Theme.popupMaxHeight : root.contentHeight
    interactive: root.scrollable
    clip: root.scrollable
    boundsBehavior: Flickable.StopAtBounds
    spacing: Theme.spacingXxs
    model: root.chapters.length
    currentIndex: -1
    highlightFollowsCurrentItem: false

    Accessible.role: Accessible.List

    ScrollBar.vertical: ScrollBar {
        visible: root.scrollable
    }

    // Keep the current chapter in view.
    onCurrentIndexChanged: {
        if (root.scrollable && root.currentIndex >= 0)
            root.positionViewAtIndex(root.currentIndex, ListView.Contain)
    }

    delegate: T.ItemDelegate {
        id: row

        required property int index

        readonly property bool current: row.index === root.currentIndex
        readonly property bool heard: root.currentIndex >= 0 && row.index < root.currentIndex
        readonly property color textColor: row.current ? Theme.textPrimary : row.heard ? Theme.textMuted : Theme.textSecondary

        width: ListView.view ? ListView.view.width - (root.scrollable ? Theme.spacingS : 0) : 0
        height: Theme.chapterRowHeight + (row.current ? Theme.spacingS : 0)
        padding: 0
        text: root.chapterTitle(row.index)

        Accessible.name: row.text

        onClicked: root.chapterActivated(row.index)

        background: Rectangle {
            radius: Theme.radiusM
            color: row.current ? Qt.alpha(Theme.accent, Theme.currentRowOpacity)
                               : row.hovered ? Theme.surface : "transparent"
        }

        contentItem: Item {
            Text {
                id: number

                x: Theme.spacingS
                width: Theme.chapterNumberWidth
                height: Theme.chapterRowHeight
                verticalAlignment: Text.AlignVCenter
                text: row.current ? "▶" : row.heard ? "✓" : String(row.index + 1)
                color: row.current ? Theme.accent : Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
            Text {
                anchors.left: number.right
                anchors.right: duration.left
                anchors.rightMargin: Theme.spacingS
                height: Theme.chapterRowHeight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                text: row.text
                color: row.textColor
                font.pixelSize: Theme.fontBody
                font.weight: row.current ? Font.DemiBold : Font.Normal
            }
            Text {
                id: duration

                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingS
                height: Theme.chapterRowHeight
                verticalAlignment: Text.AlignVCenter
                text: TimeFormat.clock(root.chapters[row.index]?.duration ?? 0)
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
            ProgressLine {
                anchors.left: number.right
                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingS
                y: Theme.chapterRowHeight - Theme.spacingXxs
                visible: row.current
                value: root.currentProgress
                color: Theme.accent
            }
        }
    }
}

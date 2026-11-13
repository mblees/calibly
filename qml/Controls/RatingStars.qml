// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Rating as five stars, read-only or editable.
// Editable: a click on a star sets the rating, a click on the current
// rating clears it; ← / → change it by one star. The stars do not change
// "value" themselves: bind it to the stored rating and store it from
// ratingEdited().
//
// Properties:
//   value     number of filled stars, 0 – 5
//   editable  the user can change the rating
// Signals:
//   ratingEdited(int value)  the user picked a rating (0 = none)
Item {
    id: root

    property int value: 0
    property bool editable: false

    // Star under the mouse while editing, -1 = none.
    property int hoveredIndex: -1
    readonly property int shownValue: root.hoveredIndex >= 0 ? root.hoveredIndex + 1 : root.value

    signal ratingEdited(int value)

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight
    activeFocusOnTab: root.editable

    Accessible.role: root.editable ? Accessible.Slider : Accessible.StaticText
    Accessible.name: qsTr("%1 von 5 Sternen").arg(root.value)

    Keys.enabled: root.editable
    Keys.onLeftPressed: root.ratingEdited(Math.max(0, root.value - 1))
    Keys.onRightPressed: root.ratingEdited(Math.min(5, root.value + 1))

    Row {
        id: row

        spacing: Theme.spacingXxs

        Repeater {
            model: 5

            delegate: Item {
                id: star

                required property int index

                width: Theme.ratingStarSize + (root.editable ? Theme.spacingXs : 0)
                height: width

                Icon {
                    anchors.centerIn: parent
                    name: "star"
                    size: Theme.ratingStarSize
                    color: star.index < root.shownValue ? Theme.rating : Theme.textDisabled
                }

                T.AbstractButton {
                    id: starButton

                    anchors.fill: parent
                    visible: root.editable
                    focusPolicy: Qt.NoFocus
                    hoverEnabled: true
                    text: qsTr("%n Stern(e)", "", star.index + 1)

                    Accessible.name: starButton.text

                    onHoveredChanged: root.hoveredIndex = starButton.hovered ? star.index : -1
                    onClicked: {
                        root.forceActiveFocus()
                        root.ratingEdited(root.value === star.index + 1 ? 0 : star.index + 1)
                    }
                }
            }
        }
    }

    // Focus frame for keyboard use.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -Theme.spacingXxs
        visible: root.activeFocus
        color: "transparent"
        radius: Theme.radiusS
        border.width: Theme.focusWidth
        border.color: Theme.textPrimary
    }
}

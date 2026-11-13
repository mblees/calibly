// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Progress bar that can be dragged or clicked to jump, e.g. the position in
// the current chapter. While the user drags, the bar follows the mouse and
// ignores changes of "value"; the new value is reported on release.
//
// Properties:
//   from, to   range
//   value      shown position; bind it to the playing position
//   large      thicker bar with a handle that is always visible
//   color      color of the passed part (accent; blue for reading)
//   dragValue  position under the mouse while dragging (read-only)
//   pressed    the user is dragging (read-only)
// Signals:
//   seekRequested(real value)  the user released the bar at this value
Item {
    id: root

    property real from: 0
    property real to: 1
    property real value: 0
    property bool large: false
    property color color: Theme.accent
    readonly property real dragValue: slider.value
    readonly property bool pressed: slider.pressed

    signal seekRequested(real value)

    implicitWidth: Theme.sliderWidth * 2
    implicitHeight: Theme.seekHandleSize + 2 * Theme.spacingXs

    Accessible.role: Accessible.Slider

    T.Slider {
        id: slider

        anchors.fill: parent
        from: root.from
        to: Math.max(root.from + 1, root.to)
        live: true
        hoverEnabled: true
        padding: 0

        // Follow the playing position, except while the user drags.
        Binding on value {
            when: !slider.pressed
            value: root.value
            restoreMode: Binding.RestoreNone
        }

        onPressedChanged: {
            if (!slider.pressed)
                root.seekRequested(slider.value)
        }
        // Keyboard and accessibility tools move the value without pressing.
        onMoved: {
            if (!slider.pressed)
                root.seekRequested(slider.value)
        }

        background: Rectangle {
            x: slider.leftPadding
            y: (slider.height - height) / 2
            width: slider.availableWidth
            height: root.large ? Theme.seekHeightLarge : Theme.seekHeight
            radius: height / 2
            color: Theme.seekTrack

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: parent.radius
                color: root.color
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * slider.availableWidth - width / 2
            y: (slider.height - height) / 2
            width: Theme.seekHandleSize
            height: width
            radius: width / 2
            color: Theme.playButton
            visible: root.large || slider.hovered || slider.pressed || slider.visualFocus
        }
    }
}

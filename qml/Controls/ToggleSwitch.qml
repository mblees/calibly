// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// On / off switch, e.g. "Fett" or "Blocksatz" in the text settings. Shows
// only the switch; the label stands next to it.
//
// Properties:
//   checked  on (inherited)
//   text     description for accessibility (inherited, not displayed)
// Signals:
//   toggled() (inherited)  the user switched it
T.Switch {
    id: control

    implicitWidth: Theme.toggleWidth
    implicitHeight: Theme.toggleHeight
    padding: 0

    Accessible.name: control.text

    indicator: Rectangle {
        width: control.width
        height: control.height
        radius: height / 2
        opacity: control.enabled ? 1 : Theme.disabledOpacity
        color: control.checked ? Theme.accent : Theme.toggleOff
        border.width: control.visualFocus ? Theme.focusWidth : 0
        border.color: Theme.textPrimary

        Behavior on color {
            ColorAnimation { duration: Theme.durationFast }
        }

        Rectangle {
            readonly property real inset: (parent.height - height) / 2

            x: control.checked ? parent.width - width - inset : inset
            y: inset
            width: Theme.toggleKnobSize
            height: Theme.toggleKnobSize
            radius: width / 2
            color: Theme.toggleKnob

            Behavior on x {
                NumberAnimation { duration: Theme.durationFast }
            }
        }
    }

    contentItem: null
}

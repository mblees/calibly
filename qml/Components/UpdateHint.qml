// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Theme
import Calibly.Controls

// Quiet hint that a newer calibly can be downloaded (release step 4).
// Presentation only.
//
// Properties:
//   version  the new version, e.g. "0.2.0-beta"
// Signals:
//   downloadRequested()  "Herunterladen": open the release page
//   dismissRequested()   "Ignorieren": no hint for this version again
Rectangle {
    id: root

    property string version

    signal downloadRequested()
    signal dismissRequested()

    implicitWidth: Theme.importQueueWidth
    implicitHeight: content.implicitHeight + 2 * Theme.spacingM
    radius: Theme.radiusL
    color: Theme.surfaceRaised
    border.width: Theme.borderWidth
    border.color: Theme.inputBorder

    Accessible.role: Accessible.AlertMessage
    Accessible.name: title.text

    Row {
        id: content

        x: Theme.spacingM
        y: Theme.spacingM
        width: parent.width - 2 * Theme.spacingM
        spacing: Theme.spacingM

        Icon {
            name: "download"
            color: Theme.accent
        }

        Column {
            width: parent.width - Theme.iconSize - parent.spacing
            spacing: Theme.spacingS

            Text {
                id: title

                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("calibly %1 ist verfügbar").arg(root.version)
                color: Theme.textPrimary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Die neue Version wird über die alte installiert; die Bibliothek bleibt erhalten.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
            Row {
                spacing: Theme.spacingM

                AppButton {
                    variant: AppButton.Primary
                    text: qsTr("Herunterladen")
                    onClicked: root.downloadRequested()
                }
                LinkButton {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Ignorieren")
                    onClicked: root.dismissRequested()
                }
            }
        }
    }
}

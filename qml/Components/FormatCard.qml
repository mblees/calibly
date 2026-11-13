// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Theme
import Calibly.Controls

// One format of a work in the detail view: type, file, size, length,
// progress and the button to continue reading or listening.
//
// Properties:
//   kind             "ebook" or "audiobook"
//   fileType         e.g. "EPUB", "M4B"
//   fileSize         bytes, 0 = unknown
//   pages            ebooks, 0 = unknown
//   durationSeconds  audiobooks, 0 = unknown
//   progress         0.0 – 1.0
//   available        the file exists; otherwise the button is disabled
//   openFailed       show that the system could not open the file
//   readsInApp       the ebook opens in calibly's reader: the card also
//                    offers "Mit Standardprogramm öffnen"
//   volumeNumber     a volume of a combined work: "Band 2" instead of
//                    "E-Book"; 0 = a single book
//   volumeTitle      its title, shown below
// Signals:
//   openRequested()     the user wants to read / listen
//   openExternallyRequested()   the file in the default application
//   finishedToggled()   the link "Als fertig markieren" was clicked
Rectangle {
    id: root

    required property string kind
    required property string fileType
    required property real fileSize
    required property int pages
    required property int durationSeconds
    required property real progress
    required property bool available
    property bool openFailed: false
    property bool readsInApp: false
    property int volumeNumber: 0
    property string volumeTitle

    signal openRequested()
    signal openExternallyRequested()
    signal finishedToggled()

    readonly property bool finished: root.progress >= 1
    readonly property bool isAudiobook: root.kind === "audiobook"
    readonly property color formatColor: root.isAudiobook ? Theme.listen : Theme.read
    readonly property int percent: Math.round(root.progress * 100)

    function durationText(seconds: real): string {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.round((seconds % 3600) / 60)
        return hours > 0 ? qsTr("%1 Std. %2 Min.").arg(hours).arg(minutes)
                         : qsTr("%1 Min.").arg(minutes)
    }

    function sizeText(bytes: real): string {
        const locale = Qt.locale(Qt.uiLanguage)
        if (bytes >= 1e9)
            return qsTr("%1 GB").arg((bytes / 1e9).toLocaleString(locale, "f", 1))
        return qsTr("%1 MB").arg((bytes / 1e6).toLocaleString(locale, "f", 1))
    }

    function detailText(): string {
        if (root.isAudiobook) {
            if (root.durationSeconds <= 0)
                return ""
            const total = root.durationText(root.durationSeconds)
            if (root.progress <= 0 || root.progress >= 1)
                return total
            return qsTr("%1 · noch %2").arg(total)
                       .arg(root.durationText(root.durationSeconds * (1 - root.progress)))
        }
        if (root.pages <= 0)
            return ""
        if (root.progress <= 0 || root.progress >= 1)
            return qsTr("%Ln Seiten", "", root.pages)
        return qsTr("%Ln Seiten · Seite %1", "", root.pages)
                   .arg(Math.max(1, Math.round(root.pages * root.progress)))
    }

    implicitWidth: Theme.formatCardWidth
    implicitHeight: content.implicitHeight + 2 * Theme.spacingM
    radius: Theme.radiusM
    color: Theme.surface
    border.width: Theme.borderWidth
    border.color: Qt.alpha(root.formatColor, Theme.formatBorderOpacity)

    Column {
        id: content

        anchors.fill: parent
        anchors.margins: Theme.spacingM
        spacing: Theme.spacingS

        Item {
            width: parent.width
            height: Math.max(heading.implicitHeight, fileInfo.implicitHeight)

            Row {
                id: heading

                spacing: Theme.spacingS

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: root.isAudiobook ? "headphones" : "book"
                    color: root.formatColor
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.isAudiobook ? qsTr("Hörbuch")
                                           : root.volumeNumber > 0 ? qsTr("Band %1").arg(root.volumeNumber)
                                                                   : qsTr("E-Book")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                }
            }
            Text {
                id: fileInfo

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: root.fileSize > 0 ? root.fileType + " · " + root.sizeText(root.fileSize) : root.fileType
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }

        Text {
            width: parent.width
            visible: root.volumeTitle !== ""
            text: root.volumeTitle
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
        Text {
            width: parent.width
            visible: text !== ""
            text: root.detailText()
            elide: Text.ElideRight
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }

        ProgressLine {
            width: parent.width
            value: root.progress
            color: root.finished ? Theme.finished : root.formatColor
        }

        AppButton {
            width: parent.width
            enabled: root.available
            variant: root.isAudiobook ? AppButton.Primary : AppButton.Read
            iconName: root.isAudiobook ? "play" : "book"
            text: {
                if (root.progress > 0 && root.progress < 1)
                    return root.isAudiobook ? qsTr("Weiterhören · %1 %").arg(root.percent)
                                            : qsTr("Weiterlesen · %1 %").arg(root.percent)
                return root.isAudiobook ? qsTr("Hören") : qsTr("Lesen")
            }

            onClicked: root.openRequested()
        }

        Text {
            width: parent.width
            visible: !root.available || root.openFailed
            text: !root.available ? qsTr("Datei nicht gefunden.")
                                  : qsTr("Kein Programm zum Öffnen gefunden.")
            wrapMode: Text.Wrap
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }

        LinkButton {
            visible: root.readsInApp && !root.isAudiobook
            enabled: root.available
            iconName: "externalLink"
            text: qsTr("Mit Standardprogramm öffnen")
            onClicked: root.openExternallyRequested()
        }

        LinkButton {
            iconName: "check"
            iconColor: root.finished ? Theme.finished : Theme.textMuted
            text: root.finished ? qsTr("Als nicht fertig markieren") : qsTr("Als fertig markieren")
            onClicked: root.finishedToggled()
        }
    }
}

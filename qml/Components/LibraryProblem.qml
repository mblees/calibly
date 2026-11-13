// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import Calibly.Theme
import Calibly.Controls

// Shown instead of the library when it cannot be opened: what happened, the
// folder, and the ways out. Presentation only.
//
// Properties:
//   reason  why, as LibraryController.libraryError:
//           LibraryProblem.NotFound      the folder or calibly.db is gone
//                                        (e.g. a drive that is not connected)
//           LibraryProblem.CannotCreate  the folder cannot be created / written
//           LibraryProblem.Damaged       calibly.db is damaged
//           LibraryProblem.NewerVersion  written by a newer calibly
//           LibraryProblem.CannotUpdate  the update to this version failed
//           LibraryProblem.InUse         another calibly has it open
//   path    the library folder
// Signals:
//   retryRequested()   "Erneut versuchen"
//   openRequested()    "Andere Bibliothek öffnen …": pick a folder with a library
//   createRequested()  "Neue Bibliothek anlegen …": pick an (empty) folder
Item {
    id: root

    enum Reason {
        NotFound = 1,
        CannotCreate,
        Damaged,
        NewerVersion,
        CannotUpdate,
        InUse
    }

    property int reason: LibraryProblem.NotFound
    property string path

    signal retryRequested()
    signal openRequested()
    signal createRequested()

    readonly property string heading: {
        switch (root.reason) {
        case LibraryProblem.CannotCreate:
            return qsTr("Die Bibliothek kann nicht angelegt werden")
        case LibraryProblem.Damaged:
            return qsTr("Die Bibliothek ist beschädigt")
        case LibraryProblem.NewerVersion:
            return qsTr("Die Bibliothek ist für eine neuere calibly-Version")
        case LibraryProblem.CannotUpdate:
            return qsTr("Die Bibliothek kann nicht aktualisiert werden")
        case LibraryProblem.InUse:
            return qsTr("Die Bibliothek ist schon geöffnet")
        default:
            return qsTr("Die Bibliothek wurde nicht gefunden")
        }
    }
    readonly property string explanation: {
        switch (root.reason) {
        case LibraryProblem.CannotCreate:
            return qsTr("Der Ordner kann nicht angelegt oder beschrieben werden. Ist das Laufwerk schreibgeschützt?")
        case LibraryProblem.Damaged:
            return qsTr("Die Datenbank calibly.db in diesem Ordner lässt sich nicht lesen. calibly verändert sie nicht. Sicherungen früherer Versionen liegen, falls vorhanden, als „calibly.db.v….bak“ im selben Ordner.")
        case LibraryProblem.NewerVersion:
            return qsTr("Sie wurde mit einer neueren Version von calibly bearbeitet. Bitte installiere die neueste Version.")
        case LibraryProblem.CannotUpdate:
            return qsTr("Diese Version von calibly muss die Bibliothek umstellen, das ist fehlgeschlagen. Die Bibliothek ist unverändert.")
        case LibraryProblem.InUse:
            return qsTr("Ein anderes calibly-Fenster arbeitet gerade mit dieser Bibliothek. Schließe es, um sie hier zu öffnen.")
        default:
            return qsTr("In diesem Ordner liegt keine calibly-Bibliothek (mehr). Liegt sie auf einem Laufwerk, das gerade nicht angeschlossen ist?")
        }
    }

    implicitWidth: Theme.confirmDialogWidth + 2 * Theme.spacingXxl
    implicitHeight: content.implicitHeight

    Column {
        id: content

        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.spacingXl, Theme.confirmDialogWidth + Theme.spacingXxl)
        spacing: Theme.spacingM

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "folder"
            size: Theme.iconSizeLarge
            color: Theme.textMuted
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: root.heading
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle
            font.weight: Font.Bold
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
            visible: root.path !== ""
            text: root.path
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: root.explanation
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
            lineHeight: Theme.lineHeightText
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: Theme.spacingS
            spacing: Theme.spacingS

            AppButton {
                visible: root.reason !== LibraryProblem.NewerVersion
                variant: AppButton.Primary
                text: qsTr("Erneut versuchen")
                onClicked: root.retryRequested()
            }
            AppButton {
                iconName: "folder"
                text: qsTr("Andere Bibliothek öffnen …")
                onClicked: root.openRequested()
            }
            AppButton {
                iconName: "plus"
                text: qsTr("Neue Bibliothek anlegen …")
                onClicked: root.createRequested()
            }
        }
    }
}

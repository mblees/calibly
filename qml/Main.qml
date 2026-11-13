// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import Calibly.Controllers
import Calibly.Models
import Calibly.Theme
import Calibly.Components
import Calibly.Views

Window {
    id: root

    // Set by main.cpp (--gallery): show the component gallery instead of the library.
    property bool galleryMode: false

    width: 1280
    height: 800
    minimumWidth: 800
    minimumHeight: 500
    visible: true
    title: root.galleryMode ? qsTr("calibly – Komponenten-Galerie") : "calibly"
    color: Theme.background

    // Navigation entries. Adding a view = one entry here + one file in qml/Views.
    //   view  QML type name in the module Calibly.Views
    ListModel {
        id: navigation

        ListElement { title: qsTr("Bibliothek"); iconName: "library"; count: 0; view: "LibraryView" }
        ListElement { title: qsTr("Autoren"); iconName: "user"; count: 0; view: "AuthorsView" }
        ListElement { title: qsTr("Serien"); iconName: "layers"; count: 0; view: "SeriesView" }
        ListElement { title: qsTr("Neu"); iconName: "star"; count: 0; view: "NewView" }
        ListElement { title: qsTr("Weitermachen"); iconName: "clock"; count: 0; view: "ContinueView" }
    }

    // Entries at the bottom of the sidebar; their indices follow navigation's.
    ListModel {
        id: bottomNavigation

        ListElement { title: qsTr("Einstellungen"); iconName: "settings"; count: 0; view: "SettingsView" }
    }

    // The library entry shows the number of works, "Autoren" the number of
    // authors, "Serien" the number of series, "Neu" the titles added in the
    // last minutes, "Weitermachen" the number of titles that were started
    // and are not finished.
    function updateCounts() {
        navigation.setProperty(0, "count", LibraryController.works.count)
        navigation.setProperty(1, "count", authorStacks.count)
        navigation.setProperty(2, "count", seriesStacks.count)
        navigation.setProperty(3, "count", newWorks.count)
        navigation.setProperty(4, "count", continueWorks.count)
    }

    Component.onCompleted: root.updateCounts()

    // The accent chosen in the settings colors the whole app.
    Binding {
        target: Theme
        property: "accent"
        value: AppearanceSettings.accentColor
    }
    Binding {
        target: Theme
        property: "accentForeground"
        value: AppearanceSettings.accentForeground
    }

    ContinueModel {
        id: continueWorks

        sourceModel: LibraryController.works

        onCountChanged: root.updateCounts()
    }

    // One row per series, like the view "Serien".
    WorkFilterModel {
        id: seriesStacks

        sourceModel: LibraryController.works
        groupSeries: true
        seriesOnly: true

        onCountChanged: root.updateCounts()
    }

    // One row per author, like the view "Autoren".
    WorkFilterModel {
        id: authorStacks

        sourceModel: LibraryController.works
        groupAuthors: true

        onCountChanged: root.updateCounts()
    }

    // The titles of the view "Neu" (see LibraryView.newMinutes).
    WorkFilterModel {
        id: newWorks

        sourceModel: LibraryController.works
        addedWithin: 10 * 60

        onCountChanged: root.updateCounts()
    }

    Connections {
        target: LibraryController.works

        function onCountChanged() {
            root.updateCounts()
        }
    }

    // calibly was started a second time with this library: come to the front.
    Connections {
        target: LibraryController

        function onActivationRequested() {
            if (root.visibility === Window.Minimized)
                root.showNormal()
            root.raise()
            root.requestActivate()
        }
    }

    // Back to the library when the reader closes, so its keys work again.
    Connections {
        target: ReaderController

        function onOpenChanged() {
            if (!ReaderController.isOpen)
                main.forceActiveFocus()
        }
    }

    // Library and player bar. Keys that no control used go to the reader
    // while a book is open, otherwise to the player: Space plays / pauses,
    // ← / → skip 30 seconds.
    ColumnLayout {
        id: main

        anchors.fill: parent
        spacing: 0
        visible: !root.galleryMode && LibraryController.libraryError === LibraryController.NoError
        focus: true

        Keys.onPressed: function(event) {
            if (ReaderController.isOpen) {
                reader.handleKey(event)
                return
            }
            if (!PlayerController.hasWork)
                return
            if (event.key === Qt.Key_Space)
                PlayerController.togglePlay()
            else if (event.key === Qt.Key_Left)
                PlayerController.skip(-30)
            else if (event.key === Qt.Key_Right)
                PlayerController.skip(30)
            else
                return
            event.accepted = true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Hidden while the reader is in fullscreen.
            SidebarNav {
                id: sidebar

                Layout.fillHeight: true
                visible: !reader.fullscreen
                model: navigation
                bottomModel: bottomNavigation

                // Another view closes the book.
                onActivated: function(index) {
                    ReaderController.close()
                    sidebar.currentIndex = index
                }
            }

            // The view of the sidebar entry; the reader lies over it while a
            // book is open, so the view keeps its place.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Loader {
                    id: content

                    readonly property var entry: sidebar.currentIndex < navigation.count
                                                 ? navigation.get(sidebar.currentIndex)
                                                 : bottomNavigation.get(sidebar.currentIndex - navigation.count)

                    anchors.fill: parent
                    visible: !ReaderController.isOpen
                    sourceComponent: Qt.createComponent("Calibly.Views", content.entry.view)

                    // Bindings (not plain assignments), so the title follows a language change.
                    onLoaded: {
                        const view = content.item as ViewBase
                        if (view) {
                            view.title = Qt.binding(() => content.entry.title)
                            view.iconName = Qt.binding(() => content.entry.iconName)
                        }
                    }
                }

                ReaderView {
                    id: reader

                    anchors.fill: parent
                    backText: content.entry.title
                }
            }
        }

        PlayerBar {
            Layout.fillWidth: true
            Layout.preferredHeight: reader.fullscreen ? 0 : implicitHeight
            visible: PlayerController.hasWork && !reader.fullscreen
        }
    }

    // A newer calibly can be downloaded (release step 4): a quiet hint at the
    // bottom left of the content area (the import queue is at the right).
    UpdateHint {
        id: updateHint

        x: sidebar.width + Theme.spacingL
        y: parent.height - height - Theme.spacingL - (PlayerController.hasWork ? Theme.playerBarHeight : 0)
        visible: !root.galleryMode && UpdateChecker.hintVisible && !reader.fullscreen
                 && LibraryController.libraryError === LibraryController.NoError
        version: UpdateChecker.latestVersion
        onDownloadRequested: Qt.openUrlExternally(UpdateChecker.releaseUrl)
        onDismissRequested: UpdateChecker.dismiss()
    }

    // Export of selected titles: progress and result, above the update hint.
    ExportHint {
        x: sidebar.width + Theme.spacingL
        y: (updateHint.visible ? updateHint.y - Theme.spacingM : parent.height - Theme.spacingL
            - (PlayerController.hasWork ? Theme.playerBarHeight : 0)) - height
        visible: !root.galleryMode && ExportController.state !== ExportController.Idle && !reader.fullscreen
        status: {
            switch (ExportController.state) {
            case ExportController.Running:
                return "running"
            case ExportController.Done:
                return "done"
            case ExportController.Cancelled:
                return "cancelled"
            default:
                return "failed"
            }
        }
        fileCount: ExportController.fileCount
        copiedCount: ExportController.copiedCount
        progress: ExportController.progress
        currentFile: ExportController.currentFile
        folderName: ExportController.folderName
        missingCount: ExportController.missingCount
        errorText: ExportController.errorText

        onCancelRequested: ExportController.cancel()
        onOpenFolderRequested: ExportController.openFolder()
        onCloseRequested: ExportController.dismiss()
    }

    // The library cannot be opened (release step 3): what happened and the
    // ways out instead of the library.
    LibraryProblem {
        anchors.fill: parent
        visible: !root.galleryMode && LibraryController.libraryError !== LibraryController.NoError
        reason: LibraryController.libraryError
        path: LibraryController.libraryPath

        onRetryRequested: LibraryController.retryLibrary()
        onOpenRequested: {
            libraryFolderDialog.create = false
            libraryFolderDialog.open()
        }
        onCreateRequested: {
            libraryFolderDialog.create = true
            libraryFolderDialog.open()
        }
    }

    FolderDialog {
        id: libraryFolderDialog

        // A new library (any folder) or an existing one (a folder with calibly.db).
        property bool create: false

        title: libraryFolderDialog.create ? qsTr("Ordner für die neue Bibliothek") : qsTr("Bibliothek öffnen")
        onAccepted: LibraryController.openLibraryUrl(libraryFolderDialog.selectedFolder, libraryFolderDialog.create)
    }

    Loader {
        anchors.fill: parent
        active: root.galleryMode
        sourceComponent: ComponentGallery {}
    }
}

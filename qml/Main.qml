pragma ComponentBehavior: Bound
import QtQuick
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
        ListElement { title: qsTr("Weitermachen"); iconName: "clock"; count: 0; view: "ContinueView" }
        ListElement { title: qsTr("Serien"); iconName: "layers"; count: 0; view: "SeriesView" }
        ListElement { title: qsTr("Autoren"); iconName: "user"; count: 0; view: "PlaceholderView" }
        ListElement { title: qsTr("Sprecher"); iconName: "mic"; count: 0; view: "PlaceholderView" }
    }

    // The library entry shows the number of works, "Weitermachen" the
    // number of titles that were started and are not finished, "Serien" the
    // number of series.
    function updateCounts() {
        navigation.setProperty(0, "count", LibraryController.works.count)
        navigation.setProperty(1, "count", continueWorks.count)
        navigation.setProperty(2, "count", seriesStacks.count)
    }

    Component.onCompleted: root.updateCounts()

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

    Connections {
        target: LibraryController.works

        function onCountChanged() {
            root.updateCounts()
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
        visible: !root.galleryMode
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

                    readonly property var entry: navigation.get(sidebar.currentIndex)

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

    Loader {
        anchors.fill: parent
        active: root.galleryMode
        sourceComponent: ComponentGallery {}
    }
}

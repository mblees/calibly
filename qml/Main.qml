pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Calibly.Controllers
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
        ListElement { title: qsTr("Weitermachen"); iconName: "clock"; count: 0; view: "PlaceholderView" }
        ListElement { title: qsTr("Serien"); iconName: "layers"; count: 0; view: "PlaceholderView" }
        ListElement { title: qsTr("Autoren"); iconName: "user"; count: 0; view: "PlaceholderView" }
        ListElement { title: qsTr("Sprecher"); iconName: "mic"; count: 0; view: "PlaceholderView" }
    }

    // The library entry shows the number of works.
    function updateCounts() {
        navigation.setProperty(0, "count", LibraryController.works.count)
    }

    Component.onCompleted: root.updateCounts()

    Connections {
        target: LibraryController.works

        function onCountChanged() {
            root.updateCounts()
        }
    }

    // Library and player bar. Keys that no control used go to the player:
    // Space plays / pauses, ← / → skip 30 seconds.
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: !root.galleryMode
        focus: true

        Keys.onPressed: function(event) {
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

            SidebarNav {
                id: sidebar

                Layout.fillHeight: true
                model: navigation

                onActivated: function(index) {
                    sidebar.currentIndex = index
                }
            }

            Loader {
                id: content

                readonly property var entry: navigation.get(sidebar.currentIndex)

                Layout.fillWidth: true
                Layout.fillHeight: true
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
        }

        PlayerBar {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
        }
    }

    Loader {
        anchors.fill: parent
        active: root.galleryMode
        sourceComponent: ComponentGallery {}
    }
}

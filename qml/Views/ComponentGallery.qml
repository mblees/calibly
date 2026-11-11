pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Calibly.Controllers
import Calibly.Theme
import Calibly.Controls
import Calibly.Components

// Developer view (start with --gallery). Shows every building block in all
// its states with mock data. New components are added here first and only
// then used in a real view.
ViewBase {
    id: root

    title: qsTr("Komponenten-Galerie")

    // A titled block of the gallery.
    component Section: Column {
        id: section

        required property string heading

        spacing: Theme.spacingM

        Text {
            text: section.heading
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1
        }
    }

    // Small caption below a sample.
    component Caption: Text {
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
    }

    Flickable {
        id: flick

        anchors.fill: parent
        contentWidth: width
        contentHeight: page.implicitHeight + 2 * Theme.spacingXxl
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        Column {
            id: page

            x: Theme.spacingXxl
            y: Theme.spacingXxl
            width: flick.width - 2 * Theme.spacingXxl
            spacing: Theme.spacingXxl

            Column {
                spacing: Theme.spacingXs

                Text {
                    text: root.title
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontDisplay
                    font.weight: Font.Bold
                }
                Text {
                    text: qsTr("Phase 0 · Theme, Icons, Buttons, Seitenleiste")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                }
            }

            Section {
                heading: qsTr("Sprache")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingM

                    Repeater {
                        model: LanguageManager.languages

                        delegate: AppButton {
                            id: languageButton

                            required property var modelData

                            text: languageButton.modelData.name
                            variant: Qt.uiLanguage === languageButton.modelData.code
                                     ? AppButton.Primary : AppButton.Secondary
                            onClicked: Qt.uiLanguage = languageButton.modelData.code
                        }
                    }
                }
                Caption {
                    text: qsTr("Wechselt die Sprache sofort, ohne Neustart.")
                }
            }

            Section {
                heading: qsTr("Farben")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingM

                    Repeater {
                        model: [
                            { name: "background", value: Theme.background },
                            { name: "sidebar", value: Theme.sidebar },
                            { name: "surface", value: Theme.surface },
                            { name: "surfaceRaised", value: Theme.surfaceRaised },
                            { name: "border", value: Theme.border },
                            { name: "textPrimary", value: Theme.textPrimary },
                            { name: "textSecondary", value: Theme.textSecondary },
                            { name: "textMuted", value: Theme.textMuted },
                            { name: "accent / listen", value: Theme.accent },
                            { name: "read", value: Theme.read },
                            { name: "finished", value: Theme.finished },
                            { name: "danger", value: Theme.danger }
                        ]

                        delegate: Column {
                            id: swatch

                            required property var modelData

                            spacing: Theme.spacingXs

                            Rectangle {
                                width: 112
                                height: 56
                                radius: Theme.radiusM
                                color: swatch.modelData.value
                                border.width: Theme.borderWidth
                                border.color: Theme.border
                            }
                            Text {
                                text: swatch.modelData.name
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSmall
                            }
                            Caption {
                                text: String(swatch.modelData.value)
                            }
                        }
                    }
                }
            }

            Section {
                heading: qsTr("Icons")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingM

                    Repeater {
                        model: Object.keys(Icons.paths)

                        delegate: Rectangle {
                            id: iconTile

                            required property string modelData

                            width: 104
                            height: 76
                            radius: Theme.radiusM
                            color: Theme.surface

                            Icon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: Theme.spacingL
                                size: 24
                                name: iconTile.modelData
                            }
                            Caption {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: Theme.spacingS
                                text: iconTile.modelData
                            }
                        }
                    }
                }
            }

            Section {
                heading: qsTr("Buttons")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingM

                    AppButton {
                        text: qsTr("Weiterhören")
                        iconName: "play"
                        variant: AppButton.Primary
                    }
                    AppButton {
                        text: qsTr("Weiterlesen")
                        iconName: "book"
                        variant: AppButton.Read
                    }
                    AppButton {
                        text: qsTr("Bearbeiten")
                        iconName: "edit"
                    }
                    AppButton {
                        text: qsTr("Hinzufügen")
                        iconName: "plus"
                        variant: AppButton.Primary
                    }
                    AppButton {
                        text: qsTr("Deaktiviert")
                        enabled: false
                    }
                    IconButton {
                        text: qsTr("Suchen")
                        iconName: "search"
                    }
                    IconButton {
                        text: qsTr("Einstellungen")
                        iconName: "settings"
                    }
                    IconButton {
                        text: qsTr("Schließen")
                        iconName: "x"
                        round: false
                    }
                }
            }

            Section {
                heading: qsTr("Seitenleiste")
                width: page.width

                Rectangle {
                    width: Theme.sidebarWidth
                    height: 260
                    radius: Theme.radiusM
                    color: Theme.sidebar
                    clip: true

                    SidebarNav {
                        id: demoNav

                        anchors.fill: parent
                        currentIndex: 1
                        model: ListModel {
                            ListElement { title: qsTr("Bibliothek"); iconName: "library"; count: 1284 }
                            ListElement { title: qsTr("Weitermachen"); iconName: "clock"; count: 6 }
                            ListElement { title: qsTr("Serien"); iconName: "layers"; count: 48 }
                            ListElement { title: qsTr("Autoren"); iconName: "user"; count: 402 }
                            ListElement { title: qsTr("Sprecher"); iconName: "mic"; count: 96 }
                        }

                        onActivated: function(index) {
                            demoNav.currentIndex = index
                        }
                    }
                }
            }
        }
    }
}

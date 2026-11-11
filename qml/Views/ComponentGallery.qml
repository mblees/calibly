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
                    text: qsTr("Alle Bausteine mit Beispieldaten")
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
                heading: qsTr("Fortschritt und Badges")
                width: page.width

                Row {
                    spacing: Theme.spacingXl

                    Column {
                        spacing: Theme.spacingS

                        ProgressLine { width: 160; value: 0.42; color: Theme.read }
                        ProgressLine { width: 160; value: 0.58; color: Theme.listen }
                        ProgressLine { width: 160; value: 1.0; color: Theme.finished }
                        Caption { text: qsTr("42 % · 58 % · 100 %") }
                    }

                    Row {
                        spacing: Theme.spacingS

                        Badge { iconName: "book"; color: Theme.read }
                        Badge { iconName: "headphones"; color: Theme.listen }
                        Badge { text: qsTr("Band 1–4") }
                    }
                }
            }

            Section {
                heading: qsTr("Cover")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingXl

                    Repeater {
                        model: [
                            { caption: qsTr("Bild 2:3"), title: "Moby Dick", author: "Herman Melville", cover: "qrc:/mock/covers/moby-dick.jpg", square: false, stacked: false },
                            { caption: qsTr("Bild quadratisch"), title: "Dracula", author: "Bram Stoker", cover: "qrc:/mock/covers/dracula.jpg", square: true, stacked: false },
                            { caption: qsTr("Ohne Bild"), title: "Der Schimmelreiter", author: "Theodor Storm", cover: "", square: false, stacked: false },
                            { caption: qsTr("Ohne Bild, quadratisch"), title: "Die Odyssee", author: "Homer", cover: "", square: true, stacked: false },
                            { caption: qsTr("Serien-Stapel"), title: "Der Hund der Baskervilles", author: "Arthur Conan Doyle", cover: "", square: false, stacked: true },
                            { caption: qsTr("Bild fehlt (Fehler)"), title: "Woyzeck", author: "Georg Büchner", cover: "qrc:/mock/covers/does-not-exist.jpg", square: false, stacked: false }
                        ]

                        delegate: Column {
                            id: coverSample

                            required property var modelData

                            spacing: Theme.spacingS

                            CoverArt {
                                width: Theme.coverTileMinWidth
                                height: Math.round(width * Theme.coverAspect)
                                title: coverSample.modelData.title
                                author: coverSample.modelData.author
                                coverUrl: coverSample.modelData.cover
                                coverIsSquare: coverSample.modelData.square
                                stacked: coverSample.modelData.stacked
                            }
                            Caption {
                                text: coverSample.modelData.caption
                            }
                        }
                    }
                }
            }

            Section {
                heading: qsTr("Cover-Kacheln")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingXl

                    Repeater {
                        model: [
                            { caption: qsTr("Nur E-Book"), title: "Faust I", author: "J. W. von Goethe", cover: "qrc:/mock/covers/faust.jpg", square: false, ebook: true, audio: false, read: 0, listen: 0, stacked: false },
                            { caption: qsTr("Nur Hörbuch"), title: "Dracula", author: "Bram Stoker", cover: "qrc:/mock/covers/dracula.jpg", square: true, ebook: false, audio: true, read: 0, listen: 0, stacked: false },
                            { caption: qsTr("Beide Formate"), title: "Moby Dick", author: "Herman Melville", cover: "qrc:/mock/covers/moby-dick.jpg", square: false, ebook: true, audio: true, read: 0, listen: 0, stacked: false },
                            { caption: qsTr("Gelesen 42 %"), title: "Faust I", author: "J. W. von Goethe", cover: "qrc:/mock/covers/faust.jpg", square: false, ebook: true, audio: false, read: 0.42, listen: 0, stacked: false },
                            { caption: qsTr("Gehört 33 %"), title: "Dracula", author: "Bram Stoker", cover: "qrc:/mock/covers/dracula.jpg", square: true, ebook: false, audio: true, read: 0, listen: 0.33, stacked: false },
                            { caption: qsTr("Lesen + Hören"), title: "Moby Dick", author: "Herman Melville", cover: "qrc:/mock/covers/moby-dick.jpg", square: false, ebook: true, audio: true, read: 0.31, listen: 0.58, stacked: false },
                            { caption: qsTr("Ohne Cover"), title: "Der Prozess", author: "Franz Kafka", cover: "", square: false, ebook: true, audio: true, read: 0.12, listen: 0, stacked: false },
                            { caption: qsTr("Serie"), title: "Der Hund der Baskervilles", author: "Arthur Conan Doyle", cover: "", square: false, ebook: true, audio: true, read: 0, listen: 0, stacked: true }
                        ]

                        delegate: Column {
                            id: tileSample

                            required property var modelData

                            spacing: Theme.spacingXs

                            CoverTile {
                                width: Theme.coverTileMinWidth
                                title: tileSample.modelData.title
                                author: tileSample.modelData.author
                                coverUrl: tileSample.modelData.cover
                                coverIsSquare: tileSample.modelData.square
                                hasEbook: tileSample.modelData.ebook
                                hasAudiobook: tileSample.modelData.audio
                                readProgress: tileSample.modelData.read
                                listenProgress: tileSample.modelData.listen
                                stacked: tileSample.modelData.stacked
                            }
                            Caption {
                                text: tileSample.modelData.caption
                            }
                        }
                    }
                }
            }

            Section {
                id: choiceSection

                property int format: 0
                property int sort: 0

                heading: qsTr("Auswahl, Suche und Sortierung")
                width: page.width

                Row {
                    spacing: Theme.spacingXl

                    ChipGroup {
                        Chip {
                            text: qsTr("Alle")
                            count: 24
                            checked: choiceSection.format === 0
                            onClicked: choiceSection.format = 0
                        }
                        Chip {
                            text: qsTr("E-Books")
                            icons: [{ name: "book", color: Theme.read }]
                            count: 17
                            checked: choiceSection.format === 1
                            onClicked: choiceSection.format = 1
                        }
                        Chip {
                            text: qsTr("Beides")
                            icons: [{ name: "book", color: Theme.read }, { name: "headphones", color: Theme.listen }]
                            count: 8
                            checked: choiceSection.format === 2
                            onClicked: choiceSection.format = 2
                        }
                    }

                    SelectMenu {
                        iconName: "sort"
                        label: qsTr("Sortieren")
                        options: [qsTr("Zuletzt geöffnet"), qsTr("Titel"), qsTr("Autor")]
                        currentIndex: choiceSection.sort
                        onActivated: function(index) {
                            choiceSection.sort = index
                        }
                    }
                }

                Row {
                    spacing: Theme.spacingXl

                    SearchField {
                        placeholderText: qsTr("Titel, Autor, Sprecher, Serie …")
                        shortcutHint: qsTr("Strg K")
                    }
                    SearchField {
                        text: "kafka"
                    }
                    AppSlider {
                        anchors.verticalCenter: parent.verticalCenter
                        from: 0
                        to: 1
                        value: 0.4
                        leftIconName: "gridSmall"
                        rightIconName: "library"
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

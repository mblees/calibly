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
                            { caption: qsTr("Serie"), title: "Der Hund der Baskervilles", author: "Arthur Conan Doyle", cover: "", square: false, ebook: true, audio: true, read: 0, listen: 0, stacked: true },
                            { caption: qsTr("Ziel beim Ablegen"), title: "Dracula", author: "Bram Stoker", cover: "qrc:/mock/covers/dracula.jpg", square: true, ebook: true, audio: false, read: 0, listen: 0, stacked: false, drop: true }
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
                                dropLabel: tileSample.modelData.drop ? qsTr("Als Hörbuch zu „%1“ hinzufügen").arg(tileSample.modelData.title) : ""
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
                id: detailSection

                property int tab: 0

                heading: qsTr("Detail-Ansicht")
                width: page.width

                Row {
                    spacing: Theme.spacingL

                    FormatCard {
                        width: Theme.formatCardWidth
                        kind: "ebook"
                        fileType: "EPUB"
                        fileSize: 1669200
                        pages: 652
                        durationSeconds: 0
                        progress: 0.31
                        available: true
                    }
                    FormatCard {
                        width: Theme.formatCardWidth
                        kind: "audiobook"
                        fileType: "M4B"
                        fileSize: 1429440000
                        pages: 0
                        durationSeconds: 89340
                        progress: 0.58
                        available: true
                        openFailed: true
                    }
                    FormatCard {
                        width: Theme.formatCardWidth
                        kind: "audiobook"
                        fileType: "MP3"
                        fileSize: 0
                        pages: 0
                        durationSeconds: 27660
                        progress: 0
                        available: false
                    }
                }

                Row {
                    spacing: Theme.spacingXl

                    RatingStars {
                        anchors.verticalCenter: parent.verticalCenter
                        value: 4
                    }
                    Badge {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Klassiker")
                        backgroundColor: Theme.surfaceRaised
                        color: Theme.textSecondary
                    }
                    Tabs {
                        width: 320
                        tabs: [qsTr("Übersicht"), qsTr("Kapitel"), qsTr("Notizen")]
                        currentIndex: detailSection.tab
                        onActivated: function(index) {
                            detailSection.tab = index
                        }
                    }
                }
            }

            Section {
                id: formSection

                property list<string> tags: [qsTr("Horror"), qsTr("Klassiker"), qsTr("Briefroman")]
                property int rating: 3
                property int language: 1

                heading: qsTr("Formularfelder")
                width: page.width

                Flow {
                    width: page.width
                    spacing: Theme.spacingXl

                    FormField {
                        width: 260
                        label: qsTr("Gemeinsames Feld")
                        iconName: "link"
                        iconColor: Theme.shared

                        InputField {
                            text: "Dracula"
                        }
                    }
                    FormField {
                        width: 260
                        label: qsTr("Hörbuch-Feld, leer")
                        iconName: "headphones"
                        iconColor: Theme.listen

                        InputField {
                            placeholderText: qsTr("Mehrere mit Komma trennen")
                        }
                    }
                    FormField {
                        width: 260
                        label: qsTr("E-Book-Feld, nur lesen")
                        iconName: "book"
                        iconColor: Theme.read

                        InputField {
                            readOnly: true
                            text: qsTr("EPUB · 512 Seiten")
                        }
                    }
                    FormField {
                        width: 260
                        label: qsTr("Auswahl")

                        SelectMenu {
                            boxed: true
                            options: [qsTr("Unbekannt"), "Deutsch", "English", "Français"]
                            currentIndex: formSection.language
                            onActivated: function(index) {
                                formSection.language = index
                            }
                        }
                    }
                    FormField {
                        width: 400
                        label: qsTr("Schlagwörter")

                        TagField {
                            tags: formSection.tags
                            placeholderText: qsTr("+ hinzufügen")
                            onTagsEdited: function(tags) {
                                formSection.tags = tags
                            }
                        }
                    }
                    FormField {
                        width: 400
                        label: qsTr("Beschreibung")

                        TextAreaField {
                            text: qsTr("Der junge Anwalt Jonathan Harker reist nach Transsilvanien, um mit dem geheimnisvollen Grafen Dracula einen Hauskauf in London abzuschließen …")
                        }
                    }
                    FormField {
                        width: 160
                        label: qsTr("Bewertung (klickbar)")

                        Item {
                            implicitHeight: Theme.controlHeight
                            height: implicitHeight

                            RatingStars {
                                anchors.verticalCenter: parent.verticalCenter
                                editable: true
                                value: formSection.rating
                                onRatingEdited: function(value) {
                                    formSection.rating = value
                                }
                            }
                        }
                    }
                }
            }

            Section {
                id: metadataSection

                // Sample values; the form reports edits, the gallery stores them here.
                property var values: ({
                    title: "Dracula", authors: "Bram Stoker", narrators: "Maren Brandt",
                    series: "", seriesIndex: 0, language: "de", publishedYear: 1897,
                    tags: ["Horror", "Klassiker"], rating: 4,
                    description: "Ein junger Anwalt reist nach Transsilvanien, um einen Hauskauf abzuschließen.",
                    ebookPublisher: "Beispielverlag", ebookIsbn: "978-3-0000-0000-2",
                    audiobookPublisher: "Beispiel Audio", audiobookIsbn: ""
                })
                property int tab: 0

                heading: qsTr("Metadaten-Formular")
                width: page.width

                Tabs {
                    width: Math.min(page.width, 720)
                    tabs: [qsTr("Allgemein"), qsTr("Hörbuch"), qsTr("E-Book")]
                    currentIndex: metadataSection.tab
                    onActivated: function(index) {
                        metadataSection.tab = index
                    }
                }

                MetadataForm {
                    width: Math.min(page.width, 720)
                    section: [MetadataForm.General, MetadataForm.Audiobook, MetadataForm.Ebook][metadataSection.tab]
                    title: metadataSection.values.title
                    authors: metadataSection.values.authors
                    narrators: metadataSection.values.narrators
                    series: metadataSection.values.series
                    seriesIndex: metadataSection.values.seriesIndex
                    language: metadataSection.values.language
                    languages: [{ code: "de", name: "Deutsch" }, { code: "en", name: "English" }]
                    publishedYear: metadataSection.values.publishedYear
                    tags: metadataSection.values.tags
                    rating: metadataSection.values.rating
                    description: metadataSection.values.description
                    ebookPublisher: metadataSection.values.ebookPublisher
                    ebookIsbn: metadataSection.values.ebookIsbn
                    audiobookPublisher: metadataSection.values.audiobookPublisher
                    audiobookIsbn: metadataSection.values.audiobookIsbn
                    ebookFileType: "EPUB"
                    ebookPages: 512
                    audiobookFileType: "M4B"
                    audiobookDurationSeconds: 55680

                    onEdited: function(field, value) {
                        const copy = Object.assign({}, metadataSection.values)
                        copy[field] = value
                        metadataSection.values = copy
                    }
                }
            }

            Section {
                id: playerSection

                // Sample state; the components report actions, the gallery applies them.
                property bool playing: true
                property real position: 372000
                property real speed: 1.25
                property int sleepMinutes: 25
                property bool sleepAtChapterEnd: false
                property int chapter: 4
                readonly property var chapters: [
                    { title: "Der Bund", start: 0, duration: 641000 },
                    { title: "Erste Nachtwache", start: 641000, duration: 427000 },
                    { title: "", start: 1068000, duration: 578000 },
                    { title: "Moby Dick", start: 1646000, duration: 1035000 },
                    { title: "Die Weiße des Wals", start: 2681000, duration: 893000 },
                    { title: "Horch!", start: 3574000, duration: 152000 },
                    { title: "Die Seekarte", start: 3726000, duration: 889000 }
                ]

                heading: qsTr("Player")
                width: page.width

                MiniPlayer {
                    width: page.width
                    title: "Moby Dick"
                    coverUrl: "qrc:/mock/covers/moby-dick-hoerbuch.jpg"
                    subtitle: "Kap. 5 · Die Weiße des Wals · Andreas Lindner"
                    playing: playerSection.playing
                    chapterPosition: playerSection.position
                    chapterDuration: 893000
                    chapterNumber: playerSection.chapter + 1
                    chapterCount: playerSection.chapters.length
                    speed: playerSection.speed
                    sleepMinutesLeft: playerSection.sleepMinutes
                    sleepAtChapterEnd: playerSection.sleepAtChapterEnd
                    onPlayToggled: playerSection.playing = !playerSection.playing
                    onSkipRequested: function(seconds) {
                        playerSection.position = Math.max(0, Math.min(893000, playerSection.position + seconds * 1000))
                    }
                    onSeekRequested: function(value) {
                        playerSection.position = value
                    }
                    onSpeedPicked: function(speed) {
                        playerSection.speed = speed
                    }
                    onSleepPicked: function(minutes) {
                        playerSection.sleepAtChapterEnd = minutes < 0
                        playerSection.sleepMinutes = Math.max(0, minutes)
                    }
                }

                MiniPlayer {
                    width: page.width
                    title: "Dracula"
                    errorText: qsTr("Die Datei konnte nicht abgespielt werden.")
                    chapterDuration: 55680000
                }
                Caption {
                    text: qsTr("Player-Leiste: spielt, mit Fehler")
                }

                Row {
                    spacing: Theme.spacingXxl

                    Column {
                        spacing: Theme.spacingL

                        PlayerControls {
                            large: true
                            playing: playerSection.playing
                            onPlayToggled: playerSection.playing = !playerSection.playing
                        }
                        SeekBar {
                            width: 320
                            large: true
                            to: 893000
                            value: playerSection.position
                            onSeekRequested: function(value) {
                                playerSection.position = value
                            }
                        }
                        PlaybackOptions {
                            speed: playerSection.speed
                            sleepMinutesLeft: playerSection.sleepMinutes
                            sleepAtChapterEnd: playerSection.sleepAtChapterEnd
                            onSpeedPicked: function(speed) {
                                playerSection.speed = speed
                            }
                            onSleepPicked: function(minutes) {
                                playerSection.sleepAtChapterEnd = minutes < 0
                                playerSection.sleepMinutes = Math.max(0, minutes)
                            }
                        }
                        Caption {
                            text: qsTr("Vollbild: Steuerung, Kapitel-Position, Tempo und Schlaftimer")
                        }
                    }

                    Column {
                        spacing: Theme.spacingS

                        Rectangle {
                            width: 380
                            height: 300
                            radius: Theme.radiusL
                            color: Theme.playerPanel

                            ChapterList {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingS
                                chapters: playerSection.chapters
                                currentIndex: playerSection.chapter
                                currentProgress: playerSection.position / 893000
                                onChapterActivated: function(index) {
                                    playerSection.chapter = index
                                }
                            }
                        }
                        Caption {
                            text: qsTr("Kapitelliste: gehört, aktuell, offen")
                        }
                    }
                }
            }

            Section {
                heading: qsTr("Import (Drag & Drop)")
                width: page.width

                Column {
                    spacing: Theme.spacingS

                    DropHint {
                        summary: "Dracula_Hoerbuch · 27 MP3"
                    }
                    Caption {
                        text: qsTr("Hinweis beim Ziehen: über freier Fläche")
                    }
                    DropHint {
                        onCover: true
                        summary: "Die Verwandlung.m4b · 1 M4B"
                    }
                    Caption {
                        text: qsTr("Hinweis beim Ziehen: über einem Cover")
                    }
                }

                Row {
                    spacing: Theme.spacingXxl

                    Column {
                        spacing: Theme.spacingS

                        // Status values as in ImportQueue.Status.
                        ImportQueue {
                            finishedCount: 5
                            model: ListModel {
                                ListElement { entryId: 1; title: "Krieg und Frieden"; status: 3; newWork: true; fileTypes: "EPUB"; fileCount: 1; merged: false; hasEbook: true; hasAudiobook: false; progress: 1; coverUrl: "qrc:/mock/covers/krieg-und-frieden.jpg"; coverIsSquare: false; workId: "w1" }
                                ListElement { entryId: 2; title: "Die Verwandlung"; status: 3; newWork: false; fileTypes: "M4B"; fileCount: 1; merged: false; hasEbook: false; hasAudiobook: true; progress: 1; coverUrl: "qrc:/mock/covers/die-verwandlung-hoerbuch.jpg"; coverIsSquare: true; workId: "w2" }
                                ListElement { entryId: 3; title: "Der Prozess"; status: 2; newWork: true; fileTypes: "MP3"; fileCount: 12; merged: true; hasEbook: false; hasAudiobook: true; progress: 0.4; coverUrl: ""; coverIsSquare: true; workId: "w3" }
                                ListElement { entryId: 4; title: "scan_042"; status: 4; newWork: true; fileTypes: "PDF"; fileCount: 1; merged: false; hasEbook: true; hasAudiobook: false; progress: 1; coverUrl: ""; coverIsSquare: false; workId: "w4" }
                                ListElement { entryId: 5; title: "Hörspiele"; status: 1; newWork: true; fileTypes: ""; fileCount: 48; merged: false; hasEbook: false; hasAudiobook: false; progress: 0.25; coverUrl: ""; coverIsSquare: false; workId: "" }
                                ListElement { entryId: 6; title: "Neue Bücher"; status: 0; newWork: true; fileTypes: ""; fileCount: 3; merged: false; hasEbook: false; hasAudiobook: false; progress: 0; coverUrl: ""; coverIsSquare: false; workId: "" }
                                ListElement { entryId: 7; title: "Moby Dick"; status: 6; newWork: true; fileTypes: "EPUB"; fileCount: 2; merged: false; hasEbook: true; hasAudiobook: false; progress: 1; coverUrl: ""; coverIsSquare: false; workId: "" }
                                ListElement { entryId: 8; title: "Faust I"; status: 5; newWork: true; fileTypes: "EPUB"; fileCount: 1; merged: false; hasEbook: true; hasAudiobook: false; progress: 1; coverUrl: ""; coverIsSquare: false; workId: "w8" }
                            }
                        }
                        Caption {
                            text: qsTr("Import-Warteschlange: fertig, ergänzt, läuft, prüfen, liest, wartet, übersprungen, Fehler")
                        }
                    }

                    Column {
                        spacing: Theme.spacingS

                        Spinner {}
                        Caption {
                            text: qsTr("Spinner")
                        }
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

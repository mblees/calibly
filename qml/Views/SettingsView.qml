pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers

// The settings (release step 4), the entry at the bottom of the sidebar:
// the library folder, the accent color (phase 11), the language, the check
// for new versions and "Über calibly". Changes apply at once; there is no "Speichern".
ViewBase {
    id: root

    // The answer of the last openLibrary(): shown below the buttons.
    property int libraryResult: LibraryController.NoError

    function libraryResultText(result: int): string {
        switch (result) {
        case LibraryController.NotFound:
            return qsTr("In diesem Ordner liegt keine calibly-Bibliothek. Mit „Neue Bibliothek anlegen …“ entsteht dort eine.")
        case LibraryController.CannotCreate:
            return qsTr("Der Ordner kann nicht angelegt oder beschrieben werden.")
        case LibraryController.Damaged:
            return qsTr("Die Bibliothek in diesem Ordner ist beschädigt.")
        case LibraryController.NewerVersion:
            return qsTr("Die Bibliothek in diesem Ordner ist für eine neuere calibly-Version.")
        case LibraryController.CannotUpdate:
            return qsTr("Die Bibliothek in diesem Ordner kann nicht umgestellt werden.")
        case LibraryController.InUse:
            return qsTr("Ein anderes calibly-Fenster arbeitet gerade mit dieser Bibliothek.")
        default:
            return ""
        }
    }

    function dayText(date: date): string {
        return date.toLocaleString(Qt.locale(Qt.uiLanguage), Locale.ShortFormat)
    }

    // A titled block of the page.
    component Section: Column {
        id: section

        required property string heading

        width: parent ? parent.width : 0
        spacing: Theme.spacingM

        Text {
            text: section.heading
            color: Theme.textPrimary
            font.pixelSize: Theme.fontLarge
            font.weight: Font.DemiBold
        }
    }

    // Explanation below a setting.
    component Hint: Text {
        width: parent ? parent.width : 0
        wrapMode: Text.Wrap
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        lineHeight: Theme.lineHeightText
    }

    FolderDialog {
        id: folderDialog

        // A new library (any folder) or an existing one.
        property bool create: false

        title: folderDialog.create ? qsTr("Ordner für die neue Bibliothek") : qsTr("Bibliothek öffnen")
        onAccepted: root.libraryResult = LibraryController.openLibraryUrl(folderDialog.selectedFolder, folderDialog.create)
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
            width: Math.min(flick.width - 2 * Theme.spacingXxl, Theme.dialogWidth)
            spacing: Theme.spacingXxl

            Text {
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: Theme.fontDisplay
                font.weight: Font.Bold
            }

            // --- library -----------------------------------------------------------
            Section {
                heading: qsTr("Bibliothek")

                Text {
                    width: parent.width
                    wrapMode: Text.WrapAnywhere
                    text: LibraryController.libraryRoot !== "" ? LibraryController.libraryRoot
                                                               : qsTr("Beispieldaten (keine Bibliothek)")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }
                Flow {
                    width: parent.width
                    spacing: Theme.spacingS

                    AppButton {
                        enabled: LibraryController.libraryRoot !== ""
                        iconName: "folder"
                        text: qsTr("Im Explorer öffnen")
                        onClicked: Qt.openUrlExternally(AppInfo.folderUrl(LibraryController.libraryRoot))
                    }
                    AppButton {
                        text: qsTr("Andere Bibliothek öffnen …")
                        onClicked: {
                            folderDialog.create = false
                            folderDialog.open()
                        }
                    }
                    AppButton {
                        iconName: "plus"
                        text: qsTr("Neue Bibliothek anlegen …")
                        onClicked: {
                            folderDialog.create = true
                            folderDialog.open()
                        }
                    }
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    wrapMode: Text.Wrap
                    text: root.libraryResultText(root.libraryResult)
                    color: Theme.danger
                    font.pixelSize: Theme.fontBody
                }
                Hint {
                    text: qsTr("Die Bibliothek ist ein Ordner mit den Dateien und der Datenbank calibly.db. Eine andere zu öffnen verschiebt keine Dateien; die bisherige bleibt, wo sie ist, und lässt sich jederzeit wieder öffnen.")
                }
            }

            // --- appearance: accent color ------------------------------------------
            Section {
                heading: qsTr("Akzentfarbe")

                Row {
                    spacing: Theme.spacingXxl

                    ColorWheel {
                        text: qsTr("Akzentfarbe")
                        color: AppearanceSettings.accentColor
                        minimumBrightness: AppearanceSettings.minimumBrightness
                        onMoved: function(color) {
                            AppearanceSettings.accentColor = color
                        }
                    }

                    Column {
                        spacing: Theme.spacingL

                        // What the accent looks like on the things it colors.
                        Row {
                            spacing: Theme.spacingM

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: Theme.colorPreviewSize
                                height: Theme.colorPreviewSize
                                radius: Theme.radiusL
                                color: Theme.accent

                                Icon {
                                    anchors.centerIn: parent
                                    name: "headphones"
                                    color: Theme.accentForeground
                                }
                            }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: Theme.spacingXs

                                Text {
                                    text: AppearanceSettings.isDefault ? qsTr("Orange (Standard)")
                                                                       : AppearanceSettings.accentColor.toString()
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                }
                                ProgressLine {
                                    width: Theme.sliderWidth
                                    value: 0.6
                                }
                            }
                        }

                        Flow {
                            width: Theme.colorSwatchSize * 4 + Theme.spacingS * 3
                            spacing: Theme.spacingS

                            Repeater {
                                model: Theme.accentSwatches

                                delegate: ColorSwatch {
                                    required property color modelData

                                    color: modelData
                                    text: modelData.toString()
                                    checked: Qt.colorEqual(AppearanceSettings.accentColor, modelData)
                                    onClicked: AppearanceSettings.accentColor = modelData
                                }
                            }
                        }

                        AppButton {
                            enabled: !AppearanceSettings.isDefault
                            text: qsTr("Auf Orange zurücksetzen")
                            onClicked: AppearanceSettings.reset()
                        }
                    }
                }
                Hint {
                    text: qsTr("Die Akzentfarbe färbt Schaltflächen, Auswahl und alles rund ums Hören. E-Books bleiben blau, Fertiges bleibt grün. Sehr dunkle Farben werden etwas aufgehellt, damit sie auf dem dunklen Hintergrund sichtbar bleiben.")
                }
            }

            // --- language ------------------------------------------------------------
            Section {
                heading: qsTr("Sprache")

                ChipGroup {
                    Chip {
                        text: qsTr("Wie das System")
                        checked: LanguageManager.followsSystem
                        onClicked: LanguageManager.followSystem()
                    }
                    Repeater {
                        model: LanguageManager.languages

                        delegate: Chip {
                            required property var modelData

                            text: modelData.name
                            checked: !LanguageManager.followsSystem && Qt.uiLanguage === modelData.code
                            onClicked: LanguageManager.chooseLanguage(modelData.code)
                        }
                    }
                }
            }

            // --- updates ------------------------------------------------------------
            Section {
                heading: qsTr("Neue Versionen")

                Row {
                    spacing: Theme.spacingM

                    ToggleSwitch {
                        id: updateSwitch

                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Beim Start nach neuen Versionen suchen")
                        checked: UpdateChecker.enabled
                        onToggled: UpdateChecker.enabled = updateSwitch.checked
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: updateSwitch.text
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                }
                Row {
                    spacing: Theme.spacingM

                    AppButton {
                        anchors.verticalCenter: parent.verticalCenter
                        enabled: UpdateChecker.state !== UpdateChecker.Checking
                        text: qsTr("Jetzt suchen")
                        onClicked: UpdateChecker.check()
                    }
                    Spinner {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: UpdateChecker.state === UpdateChecker.Checking
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            switch (UpdateChecker.state) {
                            case UpdateChecker.Checking:
                                return qsTr("Suche …")
                            case UpdateChecker.UpToDate:
                                return qsTr("calibly %1 ist aktuell.").arg(UpdateChecker.currentVersion)
                            case UpdateChecker.Available:
                                return qsTr("calibly %1 ist verfügbar.").arg(UpdateChecker.latestVersion)
                            case UpdateChecker.Failed:
                                return qsTr("Die Suche hat nicht geklappt. Besteht eine Internetverbindung?")
                            default:
                                return isNaN(UpdateChecker.lastChecked.getTime())
                                       ? "" : qsTr("Zuletzt gesucht: %1").arg(root.dayText(UpdateChecker.lastChecked))
                            }
                        }
                        color: UpdateChecker.state === UpdateChecker.Available ? Theme.accent : Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                    LinkButton {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: UpdateChecker.state === UpdateChecker.Available
                        iconName: "download"
                        text: qsTr("Herunterladen")
                        font.pixelSize: Theme.fontBody
                        onClicked: Qt.openUrlExternally(UpdateChecker.releaseUrl)
                    }
                }
                Hint {
                    text: qsTr("calibly fragt dazu bei GitHub nach der neuesten Version, höchstens einmal am Tag. Dabei werden keine persönlichen Daten gesendet. Neue Versionen werden nicht von selbst installiert.")
                }
            }

            // --- about ----------------------------------------------------------------
            Section {
                heading: qsTr("Über calibly")

                Text {
                    text: qsTr("calibly %1").arg(AppInfo.version)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }
                Hint {
                    text: qsTr("Eine Bibliothek für E-Books und Hörbücher. calibly verwendet Open-Source-Software, unter anderem Qt, FFmpeg und TagLib; die Hinweise dazu nennen alle mit ihren Lizenzen.")
                }
                // The legal notices the GNU GPL 3 (section 5 d) asks for.
                Hint {
                    text: qsTr("%1. calibly ist freie Software: Du darfst es unter den Bedingungen der GNU General Public License Version 3 weitergeben und verändern. calibly wird ohne jede Gewährleistung bereitgestellt; Einzelheiten stehen in der Lizenz.").arg(AppInfo.copyright)
                }
                Flow {
                    width: parent.width
                    spacing: Theme.spacingL

                    LinkButton {
                        enabled: AppInfo.licenseUrl.toString() !== ""
                        iconName: "book"
                        text: qsTr("Lizenz (GNU GPL 3)")
                        font.pixelSize: Theme.fontBody
                        onClicked: Qt.openUrlExternally(AppInfo.licenseUrl)
                    }
                    LinkButton {
                        iconName: "externalLink"
                        text: qsTr("Quellcode")
                        font.pixelSize: Theme.fontBody
                        onClicked: Qt.openUrlExternally(AppInfo.sourceUrl)
                    }
                    LinkButton {
                        enabled: AppInfo.noticesUrl.toString() !== ""
                        iconName: "list"
                        text: qsTr("Hinweise zu Software Dritter")
                        font.pixelSize: Theme.fontBody
                        onClicked: Qt.openUrlExternally(AppInfo.noticesUrl)
                    }
                    LinkButton {
                        enabled: AppInfo.logFolderUrl.toString() !== ""
                        iconName: "folder"
                        text: qsTr("Log-Dateien (für Fehlerberichte)")
                        font.pixelSize: Theme.fontBody
                        onClicked: Qt.openUrlExternally(AppInfo.logFolderUrl)
                    }
                    LinkButton {
                        iconName: "externalLink"
                        text: qsTr("Download-Seite")
                        font.pixelSize: Theme.fontBody
                        onClicked: Qt.openUrlExternally(AppInfo.releasesUrl)
                    }
                }
            }
        }
    }
}

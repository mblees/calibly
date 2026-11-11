pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Calibly.Theme
import Calibly.Controls

// Form with the metadata of one work (view E). One section is shown at a
// time: General (all important fields), Audiobook or Ebook. Fields marked
// with the link icon are shared by all formats; the headphones / book icon
// marks fields of the audiobook / ebook only.
//
// The form shows the values it gets and reports every change with
// edited(field, value); it never changes its values itself. field is the
// name of the property, so the view can write value straight back.
//
// Properties:
//   section        MetadataForm.General, .Audiobook or .Ebook
//   hasEbook, hasAudiobook   which format fields are shown
//   title, authors, narrators  texts; authors and narrators are separated by ","
//   series, seriesIndex (0 = none), language (code, "" = unknown),
//   languages      [{ code, name }] offered for the language
//   publishedYear (0 = unknown), tags, rating (0 – 5), description
//   ebookPublisher, ebookIsbn, audiobookPublisher, audiobookIsbn
//   ebookFileType, ebookPages, audiobookFileType, audiobookDurationSeconds
//                  shown read-only (0 / "" = unknown)
// Signals:
//   edited(string field, var value)
Item {
    id: root

    enum Section {
        General,
        Audiobook,
        Ebook
    }

    property int section: MetadataForm.General
    property bool hasEbook: true
    property bool hasAudiobook: true

    property string title
    property string authors
    property string narrators
    property string series
    property int seriesIndex: 0
    property string language
    property var languages: []
    property int publishedYear: 0
    property list<string> tags
    property int rating: 0
    property string description
    property string ebookPublisher
    property string ebookIsbn
    property string audiobookPublisher
    property string audiobookIsbn
    property string ebookFileType
    property int ebookPages: 0
    property string audiobookFileType
    property int audiobookDurationSeconds: 0

    signal edited(string field, var value)

    implicitWidth: Theme.searchFieldWidth * 2
    implicitHeight: root.section === MetadataForm.General ? general.implicitHeight
                  : root.section === MetadataForm.Audiobook ? audiobook.implicitHeight
                  : ebook.implicitHeight

    function numberText(value: int): string {
        return value > 0 ? String(value) : ""
    }

    function durationText(seconds: int): string {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.round((seconds % 3600) / 60)
        return hours > 0 ? qsTr("%1 Std. %2 Min.").arg(hours).arg(minutes) : qsTr("%1 Min.").arg(minutes)
    }

    readonly property string ebookInfo: {
        const parts = []
        if (root.ebookFileType !== "")
            parts.push(root.ebookFileType)
        if (root.ebookPages > 0)
            parts.push(qsTr("%Ln Seiten", "", root.ebookPages))
        return parts.length > 0 ? parts.join(" · ") : qsTr("unbekannt")
    }
    readonly property string audiobookInfo: {
        const parts = []
        if (root.audiobookFileType !== "")
            parts.push(root.audiobookFileType)
        if (root.audiobookDurationSeconds > 0)
            parts.push(root.durationText(root.audiobookDurationSeconds))
        return parts.length > 0 ? parts.join(" · ") : qsTr("unbekannt")
    }

    // Field kinds, each with its marker.
    component SharedField: FormField {
        // Equal preferred widths: both columns get the same share.
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        iconName: "link"
        iconColor: Theme.shared
    }
    component AudiobookField: FormField {
        // Equal preferred widths: both columns get the same share.
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        iconName: "headphones"
        iconColor: Theme.listen
    }
    component EbookField: FormField {
        // Equal preferred widths: both columns get the same share.
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        iconName: "book"
        iconColor: Theme.read
    }

    // Number input; 0 is shown empty.
    component NumberInput: InputField {
        id: numberInput

        required property string field
        property int value: 0

        text: root.numberText(numberInput.value)
        validator: IntValidator { bottom: 0; top: 9999 }
        onTextEdited: root.edited(numberInput.field, numberInput.text === "" ? 0 : parseInt(numberInput.text))
    }

    // --- General ---------------------------------------------------------------
    GridLayout {
        id: general

        width: root.width
        visible: root.section === MetadataForm.General
        columns: 2
        columnSpacing: Theme.formColumnSpacing
        rowSpacing: Theme.spacingM

        SharedField {
            Layout.columnSpan: 2
            label: qsTr("Titel")

            InputField {
                text: root.title
                placeholderText: qsTr("Titel (Pflichtfeld)")
                onTextEdited: root.edited("title", text)
            }
        }
        SharedField {
            label: qsTr("Autor")

            InputField {
                text: root.authors
                placeholderText: qsTr("Mehrere mit Komma trennen")
                onTextEdited: root.edited("authors", text)
            }
        }
        AudiobookField {
            visible: root.hasAudiobook
            label: qsTr("Sprecher")

            InputField {
                text: root.narrators
                placeholderText: qsTr("Mehrere mit Komma trennen")
                onTextEdited: root.edited("narrators", text)
            }
        }
        SharedField {
            label: qsTr("Serie · Band")

            Row {
                spacing: Theme.spacingS

                InputField {
                    width: parent.width - seriesIndexInput.width - parent.spacing
                    text: root.series
                    placeholderText: qsTr("keine Serie")
                    onTextEdited: root.edited("series", text)
                }
                NumberInput {
                    id: seriesIndexInput

                    width: Theme.controlHeight * 2
                    field: "seriesIndex"
                    value: root.seriesIndex
                    placeholderText: qsTr("Band")
                }
            }
        }
        SharedField {
            label: qsTr("Sprache · Jahr")

            Row {
                spacing: Theme.spacingS

                SelectMenu {
                    width: parent.width - yearInput.width - parent.spacing
                    boxed: true
                    options: [qsTr("Unbekannt")].concat(root.languages.map(entry => entry.name))
                    currentIndex: 1 + root.languages.findIndex(entry => entry.code === root.language)
                    onActivated: function(index) {
                        root.edited("language", index === 0 ? "" : root.languages[index - 1].code)
                    }
                }
                NumberInput {
                    id: yearInput

                    width: Theme.controlHeight * 2
                    field: "publishedYear"
                    value: root.publishedYear
                    placeholderText: qsTr("Jahr")
                }
            }
        }
        EbookField {
            visible: root.hasEbook
            label: qsTr("ISBN")

            InputField {
                text: root.ebookIsbn
                onTextEdited: root.edited("ebookIsbn", text)
            }
        }
        AudiobookField {
            visible: root.hasAudiobook
            label: qsTr("Hörbuch")

            InputField {
                readOnly: true
                text: root.audiobookInfo
            }
        }
        SharedField {
            Layout.columnSpan: 2
            label: qsTr("Schlagwörter")

            TagField {
                tags: root.tags
                placeholderText: qsTr("+ hinzufügen")
                onTagsEdited: function(tags) {
                    root.edited("tags", tags)
                }
            }
        }
        SharedField {
            Layout.columnSpan: 2
            label: qsTr("Beschreibung")

            TextAreaField {
                text: root.description
                onTextEdited: root.edited("description", text)
            }
        }
        SharedField {
            Layout.columnSpan: 2
            label: qsTr("Bewertung")

            Item {
                implicitHeight: Theme.controlHeight
                height: implicitHeight

                RatingStars {
                    anchors.verticalCenter: parent.verticalCenter
                    editable: true
                    value: root.rating
                    onRatingEdited: function(value) {
                        root.edited("rating", value)
                    }
                }
            }
        }
    }

    // --- Audiobook -------------------------------------------------------------
    GridLayout {
        id: audiobook

        width: root.width
        visible: root.section === MetadataForm.Audiobook
        columns: 2
        columnSpacing: Theme.formColumnSpacing
        rowSpacing: Theme.spacingM

        AudiobookField {
            Layout.columnSpan: 2
            label: qsTr("Sprecher")

            InputField {
                text: root.narrators
                placeholderText: qsTr("Mehrere mit Komma trennen")
                onTextEdited: root.edited("narrators", text)
            }
        }
        AudiobookField {
            label: qsTr("Verlag")

            InputField {
                text: root.audiobookPublisher
                onTextEdited: root.edited("audiobookPublisher", text)
            }
        }
        AudiobookField {
            label: qsTr("ISBN")

            InputField {
                text: root.audiobookIsbn
                onTextEdited: root.edited("audiobookIsbn", text)
            }
        }
        AudiobookField {
            Layout.columnSpan: 2
            label: qsTr("Datei · Länge")

            InputField {
                readOnly: true
                text: root.audiobookInfo
            }
        }
    }

    // --- Ebook -----------------------------------------------------------------
    GridLayout {
        id: ebook

        width: root.width
        visible: root.section === MetadataForm.Ebook
        columns: 2
        columnSpacing: Theme.formColumnSpacing
        rowSpacing: Theme.spacingM

        EbookField {
            label: qsTr("Verlag")

            InputField {
                text: root.ebookPublisher
                onTextEdited: root.edited("ebookPublisher", text)
            }
        }
        EbookField {
            label: qsTr("ISBN")

            InputField {
                text: root.ebookIsbn
                onTextEdited: root.edited("ebookIsbn", text)
            }
        }
        EbookField {
            Layout.columnSpan: 2
            label: qsTr("Datei · Umfang")

            InputField {
                readOnly: true
                text: root.ebookInfo
            }
        }
    }
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// View E of the mockup: edit the metadata of one work. Opens as a dialog
// above everything with openFor(); changes reach the library only with
// "Speichern". ‹ / › go to the previous / next title of the given list and
// keep the changes made so far (like calibre). Esc, ✕ and "Abbrechen" drop
// unsaved changes.
//
// Properties:
//   navigationModel  the list to step through, e.g. the filtered gallery (optional)
T.Popup {
    id: root

    property WorkFilterModel navigationModel

    // Row of the current work in navigationModel, -1 if it is not in the list.
    // Depends on "revision" and the count, so it is looked up again after
    // every save (a new title may move the work) and when the list changes.
    property int revision: 0
    readonly property int row: root.navigationModel
                               ? root.rowOf(editor.workId, root.revision, root.navigationModel.count) : -1
    property string errorText

    // revision and count are only passed so the binding of "row" depends on them.
    function rowOf(workId: string, revision: int, count: int): int {
        return workId !== "" && count > 0 && revision >= 0 ? root.navigationModel.rowOf(workId) : -1
    }

    function openFor(workId: string) {
        editor.workId = ""  // reload, even if the same work was edited before
        editor.workId = workId
        sectionTabs.currentIndex = 0
        root.errorText = ""
        root.open()
    }

    // Stores the changes; false (with a message) if that is not possible.
    function save(): bool {
        if (!editor.modified)
            return true
        if (editor.title.trim() === "") {
            root.errorText = qsTr("Der Titel darf nicht leer sein.")
            return false
        }
        if (!editor.save()) {
            root.errorText = qsTr("Die Änderungen konnten nicht gespeichert werden.")
            return false
        }
        root.errorText = ""
        root.revision++
        return true
    }

    function step(offset: int) {
        const target = root.row + offset
        if (root.row < 0 || target < 0 || target >= root.navigationModel.count || !root.save())
            return
        editor.workId = root.navigationModel.workIdAt(target)
        root.errorText = ""
        if (sectionTabs.currentIndex >= root.sections.length)
            sectionTabs.currentIndex = 0
    }

    function cancel() {
        editor.revert()
        root.close()
    }

    parent: T.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(Theme.dialogWidth, parent ? parent.width - 2 * Theme.spacingXl : Theme.dialogWidth)
    height: Math.min(Theme.dialogHeight, parent ? parent.height - 2 * Theme.spacingXl : Theme.dialogHeight)
    padding: 0
    modal: true
    focus: true
    // No closing by a click outside: that would drop the changes too easily.
    closePolicy: T.Popup.CloseOnEscape

    onClosed: editor.revert()

    WorkEditor {
        id: editor
    }

    WorkDetails {
        id: details

        model: LibraryController.works
        workId: editor.workId
    }

    // Values of the first ebook / audiobook format, for the read-only fields.
    readonly property var ebookFormat: details.formats.find(format => format.kind === "ebook") ?? null
    readonly property var audiobookFormat: details.formats.find(format => format.kind === "audiobook") ?? null

    // Tabs depend on the formats of the work.
    readonly property var sections: {
        const result = [{ title: qsTr("Allgemein"), section: MetadataForm.General }]
        if (editor.hasAudiobook)
            result.push({ title: qsTr("Hörbuch"), section: MetadataForm.Audiobook })
        if (editor.hasEbook)
            result.push({ title: qsTr("E-Book"), section: MetadataForm.Ebook })
        return result
    }

    Shortcut {
        sequences: [StandardKey.Save]
        enabled: root.opened
        onActivated: {
            if (root.save())
                root.close()
        }
    }
    Shortcut {
        sequence: "Ctrl+PgUp"
        enabled: root.opened
        onActivated: root.step(-1)
    }
    Shortcut {
        sequence: "Ctrl+PgDown"
        enabled: root.opened
        onActivated: root.step(1)
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    background: Rectangle {
        radius: Theme.radiusL
        color: Theme.dialog
        border.width: Theme.borderWidth
        border.color: Theme.inputBorder
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
    }

    contentItem: Item {
        // --- header ----------------------------------------------------------
        Item {
            id: header

            width: parent.width
            height: Theme.controlHeight + 2 * Theme.spacingM

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingL
                anchors.right: navigation.left
                anchors.rightMargin: Theme.spacingL
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingS

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "edit"
                    color: Theme.textSecondary
                }
                Text {
                    id: heading

                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Metadaten bearbeiten")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(0, parent.width - heading.width - Theme.iconSize - 2 * parent.spacing)
                    elide: Text.ElideRight
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                    text: {
                        const formats = editor.hasEbook && editor.hasAudiobook ? qsTr("E-Book + Hörbuch")
                                      : editor.hasAudiobook ? qsTr("Hörbuch") : qsTr("E-Book")
                        return "· " + (details.title !== "" ? details.title + " · " : "") + formats
                    }
                }
            }

            Row {
                id: navigation

                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingM
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingXs

                IconButton {
                    visible: root.row >= 0
                    enabled: root.row > 0
                    opacity: enabled ? 1 : Theme.disabledOpacity
                    iconName: "chevronLeft"
                    text: qsTr("Voriger Titel")
                    onClicked: root.step(-1)
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.row >= 0
                    text: qsTr("%1 / %2").arg(root.row + 1).arg(root.navigationModel ? root.navigationModel.count : 0)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                }
                IconButton {
                    visible: root.row >= 0
                    enabled: root.navigationModel !== null && root.row < root.navigationModel.count - 1
                    opacity: enabled ? 1 : Theme.disabledOpacity
                    iconName: "chevronRight"
                    text: qsTr("Nächster Titel")
                    onClicked: root.step(1)
                }
                IconButton {
                    iconName: "x"
                    iconColor: Theme.textPrimary
                    text: qsTr("Schließen")
                    onClicked: root.cancel()
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: Theme.borderWidth
                color: Theme.border
            }
        }

        // --- body: cover and form --------------------------------------------
        CoverArt {
            id: cover

            anchors.left: parent.left
            anchors.top: header.bottom
            anchors.margins: Theme.spacingXl
            width: Theme.editorCoverWidth
            height: details.coverIsSquare ? width : Math.round(width * Theme.coverAspect)
            title: details.title
            author: details.authors.join(", ")
            coverUrl: details.coverUrl
            coverIsSquare: false // the frame already has the cover's shape
        }

        Tabs {
            id: sectionTabs

            anchors.left: cover.right
            anchors.right: parent.right
            anchors.top: header.bottom
            anchors.leftMargin: Theme.spacingXl
            anchors.rightMargin: Theme.spacingXl
            anchors.topMargin: Theme.spacingM
            tabs: root.sections.map(entry => entry.title)
            onActivated: function(index) {
                sectionTabs.currentIndex = index
            }
        }

        Flickable {
            id: flick

            anchors.left: sectionTabs.left
            anchors.right: parent.right
            anchors.top: sectionTabs.bottom
            anchors.bottom: footer.top
            anchors.topMargin: Theme.spacingM
            contentWidth: width
            contentHeight: form.implicitHeight + 2 * Theme.focusRingWidth + Theme.spacingL
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}

            MetadataForm {
                id: form

                // Room for the focus glow of the fields.
                x: Theme.focusRingWidth
                y: Theme.focusRingWidth
                width: flick.width - Theme.spacingXl - Theme.focusRingWidth
                section: root.sections[sectionTabs.currentIndex]?.section ?? MetadataForm.General
                hasEbook: editor.hasEbook
                hasAudiobook: editor.hasAudiobook
                title: editor.title
                authors: editor.authors
                narrators: editor.narrators
                series: editor.series
                seriesIndex: editor.seriesIndex
                language: editor.language
                languages: editor.languages
                publishedYear: editor.publishedYear
                tags: editor.tags
                rating: editor.rating
                description: editor.description
                ebookPublisher: editor.ebookPublisher
                ebookIsbn: editor.ebookIsbn
                audiobookPublisher: editor.audiobookPublisher
                audiobookIsbn: editor.audiobookIsbn
                ebookFileType: root.ebookFormat ? root.ebookFormat.fileType : ""
                ebookPages: root.ebookFormat ? root.ebookFormat.pages : 0
                audiobookFileType: root.audiobookFormat ? root.audiobookFormat.fileType : ""
                audiobookDurationSeconds: root.audiobookFormat ? root.audiobookFormat.durationSeconds : 0

                // The field names are the property names of WorkEditor.
                onEdited: function(field, value) {
                    editor[field] = value
                    root.errorText = ""
                }
            }
        }

        // --- footer ----------------------------------------------------------
        Item {
            id: footer

            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.controlHeight + 2 * Theme.spacingM

            Rectangle {
                width: parent.width
                height: Theme.borderWidth
                color: Theme.border
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingL
                anchors.right: buttons.left
                anchors.rightMargin: Theme.spacingL
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingXs

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: root.errorText !== "" ? "x" : "link"
                    size: Theme.iconSizeSmall
                    color: root.errorText !== "" ? Theme.danger : Theme.shared
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - Theme.iconSizeSmall - parent.spacing
                    elide: Text.ElideRight
                    text: root.errorText !== "" ? root.errorText : qsTr("= gilt für E-Book und Hörbuch")
                    color: root.errorText !== "" ? Theme.danger : Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
            }

            Row {
                id: buttons

                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingL
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingS

                AppButton {
                    text: qsTr("Abbrechen")
                    onClicked: root.cancel()
                }
                AppButton {
                    text: qsTr("Speichern")
                    variant: AppButton.Primary
                    enabled: editor.modified
                    onClicked: {
                        if (root.save())
                            root.close()
                    }
                }
            }
        }
    }
}

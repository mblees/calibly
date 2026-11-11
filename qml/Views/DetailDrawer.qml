pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers
import Calibly.Models

// View B of the mockup: details of one work, sliding in from the right.
// Open with openFor(); closes with Esc, ✕ or a click outside.
SideDrawer {
    id: root

    property string workId
    // The cover shown in the grid (it follows the format selection).
    property url coverUrl
    property bool coverIsSquare: false

    function openFor(workId: string, coverUrl: url, coverIsSquare: bool) {
        root.workId = workId
        root.coverUrl = coverUrl
        root.coverIsSquare = coverIsSquare
        root.open()
    }

    WorkDetails {
        id: details

        model: LibraryController.works
        workId: root.workId
    }

    contentItem: Flickable {
        id: flick

        contentWidth: width
        contentHeight: page.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        Column {
            id: page

            width: flick.width

            // --- header: blurred cover, cover, title and facts -------------------
            Item {
                id: hero

                width: parent.width
                height: Math.max(cover.height, info.implicitHeight) + 2 * Theme.spacingXl
                clip: true

                // Blurred, darkened cover behind the header (only with a cover image).
                Image {
                    anchors.fill: parent
                    anchors.margins: -Theme.coverBlurMax // no fading at the edges
                    visible: status === Image.Ready
                    source: root.coverUrl
                    asynchronous: true
                    fillMode: Image.PreserveAspectCrop
                    sourceSize.width: Theme.coverDecodeWidth / 4
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        autoPaddingEnabled: false
                        blurEnabled: true
                        blur: 1.0
                        blurMax: Theme.coverBlurMax
                        brightness: Theme.heroBlurBrightness
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    color: Qt.alpha(Theme.background, Theme.heroOverlayOpacity)
                }

                IconButton {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: Theme.spacingM
                    iconName: "x"
                    iconColor: Theme.textPrimary
                    text: qsTr("Schließen")
                    onClicked: root.close()
                }

                CoverArt {
                    id: cover

                    x: Theme.spacingXl
                    y: Theme.spacingXl
                    width: Theme.detailCoverWidth
                    height: root.coverIsSquare ? width : Math.round(width * Theme.coverAspect)
                    title: details.title
                    author: details.authors.join(", ")
                    coverUrl: root.coverUrl
                    coverIsSquare: false // the frame already has the cover's shape
                }

                Column {
                    id: info

                    anchors.left: cover.right
                    anchors.right: parent.right
                    anchors.top: cover.top
                    anchors.leftMargin: Theme.spacingXl
                    anchors.rightMargin: Theme.spacingXl + Theme.controlHeight
                    spacing: Theme.spacingS

                    Text {
                        width: parent.width
                        text: details.title
                        wrapMode: Text.WordWrap
                        color: Theme.textPrimary
                        font.family: Theme.fontFamilySerif
                        font.pixelSize: Theme.fontDisplay
                        font.weight: Font.Bold
                    }
                    Text {
                        width: parent.width
                        visible: details.authors.length > 0
                        text: qsTr("von %1").arg(details.authors.join(", "))
                        wrapMode: Text.WordWrap
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontBody
                    }
                    Text {
                        width: parent.width
                        visible: details.narrators.length > 0
                        text: qsTr("gelesen von %1").arg(details.narrators.join(", "))
                        wrapMode: Text.WordWrap
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                    Row {
                        visible: details.series !== ""
                        spacing: Theme.spacingS

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "layers"
                            color: Theme.textSecondary
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: details.seriesIndex > 0
                                  ? qsTr("%1 · Band %2").arg(details.series).arg(details.seriesIndex)
                                  : details.series
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontBody
                        }
                    }
                    RatingStars {
                        visible: details.rating > 0
                        value: details.rating
                    }
                    Flow {
                        width: parent.width
                        visible: details.tags.length > 0
                        spacing: Theme.spacingXs

                        Repeater {
                            model: details.tags

                            delegate: Badge {
                                required property string modelData

                                text: modelData
                                backgroundColor: Theme.surfaceRaised
                                color: Theme.textSecondary
                            }
                        }
                    }
                }
            }

            // --- body ------------------------------------------------------------
            Column {
                x: Theme.spacingXl
                width: parent.width - 2 * Theme.spacingXl
                topPadding: Theme.spacingL
                bottomPadding: Theme.spacingXl
                spacing: Theme.spacingL

                Flow {
                    id: cards

                    width: parent.width
                    spacing: Theme.spacingM

                    Repeater {
                        model: details.formats

                        delegate: FormatCard {
                            id: card

                            required property var modelData

                            width: details.formats.length > 1 ? (cards.width - cards.spacing) / 2 : cards.width
                            kind: modelData.kind
                            fileType: modelData.fileType
                            fileSize: modelData.fileSize
                            pages: modelData.pages
                            durationSeconds: modelData.durationSeconds
                            progress: modelData.progress
                            available: modelData.available

                            onOpenRequested: {
                                card.openFailed = !LibraryController.openFormat(root.workId, card.kind)
                            }
                        }
                    }
                }

                // Only the overview for now; chapters, notes and files follow later.
                Tabs {
                    width: parent.width
                    tabs: [qsTr("Übersicht")]
                    currentIndex: 0
                }

                Text {
                    width: parent.width
                    visible: details.description !== ""
                    text: details.description
                    wrapMode: Text.WordWrap
                    lineHeight: Theme.lineHeightText
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }

                Grid {
                    id: facts

                    readonly property var entries: {
                        const result = []
                        if (details.publishedYear > 0)
                            result.push({ label: qsTr("Erschienen"), value: String(details.publishedYear) })
                        if (details.languageName !== "")
                            result.push({ label: qsTr("Sprache"), value: details.languageName })
                        if (details.publisher !== "")
                            result.push({ label: qsTr("Verlag"), value: details.publisher })
                        if (!isNaN(details.added.getTime()))
                            result.push({ label: qsTr("Hinzugefügt"),
                                          value: details.added.toLocaleDateString(Qt.locale(Qt.uiLanguage), Locale.ShortFormat) })
                        return result
                    }

                    columns: 4
                    columnSpacing: Theme.spacingL
                    rowSpacing: Theme.spacingS

                    Repeater {
                        model: facts.entries.length * 2

                        delegate: Text {
                            required property int index

                            readonly property var entry: facts.entries[Math.floor(index / 2)]

                            text: index % 2 === 0 ? entry.label : entry.value
                            color: index % 2 === 0 ? Theme.textMuted : Theme.textPrimary
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                }
            }
        }
    }
}

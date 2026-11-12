pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Calibly.Theme
import Calibly.Controls

// Import queue at the bottom right of the library (view D): one row per
// work that is being imported, with its status. Presentation only.
//
// Properties:
//   model          rows with the roles entryId, title, status (ImportQueue.Status),
//                  newWork, fileTypes ("MP3", "EPUB + M4B"), fileCount, merged,
//                  hasEbook, hasAudiobook, progress (0 – 1), coverUrl,
//                  coverIsSquare, workId – e.g. ImportController.queue
//   finishedCount  rows that are no longer running ("2 von 4 fertig")
// Signals:
//   reviewRequested(int entryId, string workId)  "Prüfen" on a row whose title is unknown
//   entryActivated(string workId, url coverUrl, bool coverIsSquare)
//                                                click on an imported row
//   closeRequested()                             ✕ in the header
Rectangle {
    id: root

    // Same order as ImportQueueModel.Status.
    enum Status {
        Waiting,
        Reading,
        Copying,
        Done,
        NeedsReview,
        Failed,
        Skipped
    }

    property var model
    property int finishedCount: 0

    readonly property int count: list.count

    signal reviewRequested(int entryId, string workId)
    signal entryActivated(string workId, url coverUrl, bool coverIsSquare)
    signal closeRequested()

    implicitWidth: Theme.importQueueWidth
    implicitHeight: headerBar.height + list.height + Theme.spacingS
    radius: Theme.radiusL
    color: Theme.dialog
    border.width: Theme.borderWidth
    border.color: Theme.border

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Import")

    // --- header --------------------------------------------------------------
    Item {
        id: headerBar

        width: parent.width
        height: Theme.controlHeight + Theme.spacingS

        Icon {
            id: headerIcon

            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingM
            anchors.verticalCenter: parent.verticalCenter
            name: "download"
            size: Theme.iconSize - Theme.spacingXxs
            color: Theme.textSecondary
        }
        Text {
            anchors.left: headerIcon.right
            anchors.leftMargin: Theme.spacingS
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Import")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontBody
            font.weight: Font.Bold
        }
        Text {
            anchors.right: closeButton.left
            anchors.rightMargin: Theme.spacingXs
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("%1 von %2 fertig").arg(root.finishedCount).arg(root.count)
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
        IconButton {
            id: closeButton

            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingXs
            anchors.verticalCenter: parent.verticalCenter
            iconName: "x"
            iconSize: Theme.iconSizeSmall + Theme.spacingXxs
            text: qsTr("Fertige Einträge ausblenden")
            onClicked: root.closeRequested()
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.borderWidth
            color: Theme.border
        }
    }

    // --- rows ----------------------------------------------------------------
    ListView {
        id: list

        anchors.top: headerBar.bottom
        anchors.topMargin: Theme.spacingXs
        width: parent.width
        height: Math.min(list.contentHeight, Theme.importQueueMaxHeight)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.model

        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            id: row

            required property int entryId
            required property string title
            required property int status
            required property bool newWork
            required property string fileTypes
            required property int fileCount
            required property bool merged
            required property bool hasEbook
            required property bool hasAudiobook
            required property real progress
            required property url coverUrl
            required property bool coverIsSquare
            required property string workId

            readonly property bool running: row.status === ImportQueue.Waiting
                                            || row.status === ImportQueue.Reading
                                            || row.status === ImportQueue.Copying
            readonly property bool canOpen: row.workId !== "" && !row.running
                                            && row.status !== ImportQueue.Skipped
            readonly property string shownTitle: row.title !== "" ? row.title : qsTr("Unbenannt")
            // "27 MP3" for several files of one type, otherwise the types.
            readonly property string typesText: row.fileCount > 1 && row.fileTypes.indexOf("+") < 0
                                                ? row.fileCount + " " + row.fileTypes : row.fileTypes
            readonly property string detailText: {
                switch (row.status) {
                case ImportQueue.Waiting:
                    return qsTr("wartet …")
                case ImportQueue.Reading:
                    return row.fileCount > 0 ? qsTr("%n Datei(en) werden gelesen …", "", row.fileCount)
                                             : qsTr("Dateien werden gesucht …")
                case ImportQueue.Copying:
                    return row.merged ? qsTr("%1 werden zusammengefasst …").arg(row.typesText)
                                      : qsTr("%1 wird kopiert …").arg(row.typesText)
                case ImportQueue.Done:
                    if (row.newWork)
                        return qsTr("%1 → neuer Titel").arg(row.typesText)
                    if (row.hasEbook && row.hasAudiobook)
                        return qsTr("%1 → Formate ergänzt").arg(row.typesText)
                    return row.hasAudiobook ? qsTr("%1 → als Hörbuch ergänzt").arg(row.typesText)
                                            : qsTr("%1 → als E-Book ergänzt").arg(row.typesText)
                case ImportQueue.NeedsReview:
                    return qsTr("Titel unbekannt – bitte prüfen")
                case ImportQueue.Failed:
                    return qsTr("konnte nicht importiert werden")
                default:
                    return row.fileCount > 0
                           ? qsTr("%n Datei(en) bereits in der Bibliothek", "", row.fileCount)
                           : qsTr("keine unterstützten Dateien")
                }
            }

            width: ListView.view.width
            height: Theme.importThumbWidth * Theme.coverAspect + 2 * Theme.spacingS

            Accessible.role: Accessible.ListItem
            Accessible.name: row.shownTitle + ", " + row.detailText

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingXs
                anchors.rightMargin: Theme.spacingXs
                radius: Theme.radiusM
                color: rowHover.hovered && row.canOpen ? Theme.hoverTint : "transparent"
            }

            CoverArt {
                id: thumb

                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingM
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.importThumbWidth
                height: Math.round(width * Theme.coverAspect)
                title: row.shownTitle
                coverUrl: row.coverUrl
                coverIsSquare: row.coverIsSquare
            }

            Column {
                anchors.left: thumb.right
                anchors.leftMargin: Theme.spacingM
                anchors.right: statusItem.left
                anchors.rightMargin: Theme.spacingS
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingXxs

                Text {
                    width: parent.width
                    text: row.shownTitle
                    elide: Text.ElideRight
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                }
                Text {
                    width: parent.width
                    text: row.detailText
                    elide: Text.ElideRight
                    color: row.status === ImportQueue.Failed ? Theme.danger : Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
                ProgressLine {
                    width: parent.width
                    visible: row.status === ImportQueue.Reading || row.status === ImportQueue.Copying
                    value: row.progress
                }
            }

            // Status at the right: spinner, "fertig", "Prüfen" …
            Item {
                id: statusItem

                anchors.right: parent.right
                anchors.rightMargin: Theme.spacingM
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(statusRow.implicitWidth, reviewButton.implicitWidth)
                height: Theme.controlHeight

                Spinner {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: row.status === ImportQueue.Reading || row.status === ImportQueue.Copying
                }

                Row {
                    id: statusRow

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.spacingXs
                    visible: row.status === ImportQueue.Done || row.status === ImportQueue.Failed
                             || row.status === ImportQueue.Skipped || row.status === ImportQueue.Waiting

                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: row.status === ImportQueue.Done || row.status === ImportQueue.Failed
                        name: row.status === ImportQueue.Failed ? "alert" : "check"
                        size: Theme.iconSizeSmall + Theme.spacingXxs
                        strokeWidth: Theme.badgeIconStrokeWidth
                        color: row.status === ImportQueue.Failed ? Theme.danger : Theme.finished
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            switch (row.status) {
                            case ImportQueue.Done:
                                return qsTr("fertig")
                            case ImportQueue.Failed:
                                return qsTr("Fehler")
                            case ImportQueue.Skipped:
                                return qsTr("übersprungen")
                            default:
                                return qsTr("wartet")
                            }
                        }
                        color: row.status === ImportQueue.Done ? Theme.finished
                               : row.status === ImportQueue.Failed ? Theme.danger : Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                    }
                }

                AppButton {
                    id: reviewButton

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: row.status === ImportQueue.NeedsReview
                    text: qsTr("Prüfen")
                    variant: AppButton.Primary
                    leftPadding: Theme.spacingM
                    rightPadding: Theme.spacingM
                    onClicked: root.reviewRequested(row.entryId, row.workId)
                }
            }

            HoverHandler {
                id: rowHover

                cursorShape: row.canOpen ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
            TapHandler {
                enabled: row.canOpen
                onTapped: {
                    if (row.status === ImportQueue.NeedsReview)
                        root.reviewRequested(row.entryId, row.workId)
                    else
                        root.entryActivated(row.workId, row.coverUrl, row.coverIsSquare)
                }
            }
        }
    }
}

import QtQuick
import Calibly.Theme
import Calibly.Controls

// Box at the bottom left while and after selected titles are exported
// ("Exportieren …"): progress, then the result. Presentation only.
//
// Properties:
//   status        "running", "done", "cancelled" or "failed"
//   fileCount     files to copy
//   copiedCount   files copied so far
//   progress      0 – 1
//   currentFile   name of the file being copied
//   folderName    name of the export folder
//   missingCount  files that were not found and are left out
//   errorText     why it failed
// Signals:
//   cancelRequested()      "Abbrechen" while running
//   openFolderRequested()  "Ordner öffnen"
//   closeRequested()       ✕: hide the box
Rectangle {
    id: root

    property string status: "running"
    property int fileCount: 0
    property int copiedCount: 0
    property real progress: 0
    property string currentFile
    property string folderName
    property int missingCount: 0
    property string errorText

    readonly property bool running: root.status === "running"

    signal cancelRequested()
    signal openFolderRequested()
    signal closeRequested()

    implicitWidth: Theme.importQueueWidth
    implicitHeight: content.implicitHeight + 2 * Theme.spacingM
    radius: Theme.radiusL
    color: Theme.surfaceRaised
    border.width: Theme.borderWidth
    border.color: Theme.inputBorder

    Accessible.role: Accessible.AlertMessage
    Accessible.name: title.text

    Row {
        id: content

        x: Theme.spacingM
        y: Theme.spacingM
        width: parent.width - 2 * Theme.spacingM
        spacing: Theme.spacingM

        Icon {
            name: root.status === "done" ? "check" : root.status === "failed" ? "alert" : "upload"
            color: root.status === "done" ? Theme.finished : root.status === "failed" ? Theme.danger : Theme.accent
        }

        Column {
            width: parent.width - Theme.iconSize - parent.spacing - (closeButton.visible ? closeButton.width + parent.spacing : 0)
            spacing: Theme.spacingS

            Text {
                id: title

                width: parent.width
                wrapMode: Text.Wrap
                text: {
                    switch (root.status) {
                    case "running":
                        return qsTr("%Ln Dateien werden exportiert …", "", root.fileCount)
                    case "done":
                        return qsTr("%Ln Dateien exportiert", "", root.copiedCount)
                    case "cancelled":
                        return qsTr("Export abgebrochen")
                    default:
                        return qsTr("Export fehlgeschlagen")
                    }
                }
                color: Theme.textPrimary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }

            // While running: the file and how far.
            ProgressLine {
                width: parent.width
                visible: root.running
                value: root.progress
                trackColor: Theme.inputBorder
            }
            Text {
                width: parent.width
                visible: root.running
                elide: Text.ElideMiddle
                text: qsTr("%1 von %2 · %3").arg(root.copiedCount).arg(root.fileCount).arg(root.currentFile)
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }

            // Afterwards: where, what was left out, what went wrong.
            Text {
                width: parent.width
                visible: text !== ""
                wrapMode: Text.Wrap
                text: {
                    switch (root.status) {
                    case "done":
                        return qsTr("In „%1“, aufgeteilt in Ebooks und Audiobooks.").arg(root.folderName)
                    case "cancelled":
                        return qsTr("Der angefangene Export-Ordner wurde wieder gelöscht.")
                    case "failed":
                        return root.errorText
                    default:
                        return ""
                    }
                }
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
            Text {
                width: parent.width
                visible: root.missingCount > 0 && root.status !== "cancelled"
                wrapMode: Text.Wrap
                text: qsTr("%Ln Dateien wurden nicht gefunden und fehlen im Export.", "", root.missingCount)
                color: Theme.danger
                font.pixelSize: Theme.fontSmall
            }

            Row {
                spacing: Theme.spacingM

                AppButton {
                    visible: root.status === "done" || (root.status === "failed" && root.copiedCount > 0)
                    variant: AppButton.Primary
                    iconName: "folder"
                    text: qsTr("Ordner öffnen")
                    onClicked: root.openFolderRequested()
                }
                LinkButton {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.running
                    text: qsTr("Abbrechen")
                    onClicked: root.cancelRequested()
                }
            }
        }

        IconButton {
            id: closeButton

            visible: !root.running
            iconName: "x"
            iconSize: Theme.iconSizeSmall + Theme.spacingXxs
            text: qsTr("Schließen")
            onClicked: root.closeRequested()
        }
    }
}

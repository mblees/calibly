pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import Calibly.Theme

// Cover of a work in the uniform 2:3 frame with rounded corners.
//   * 2:3 cover image: fills the frame.
//   * Square cover image (typical for audiobooks): centered, the space above
//     and below is filled with a blurred, darkened copy of the cover.
//   * No image: generated placeholder with title and author, colors derived
//     from the title.
// Child items (e.g. progress lines) are drawn on top and clipped to the
// rounded frame.
//
// Properties:
//   title, author   used for the placeholder
//   coverUrl        image, empty = placeholder
//   coverIsSquare   square cover (or square placeholder for pure audiobooks)
//   stacked         draw as a stack of sheets (series)
//   titleInset      space to keep free above the placeholder title, e.g. for
//                   a label at the top left of the cover
Item {
    id: root

    property string title
    property string author
    property url coverUrl
    property bool coverIsSquare: false
    property bool stacked: false
    property real titleInset: 0

    default property alias overlay: overlayItem.data

    readonly property bool hasImage: root.coverUrl.toString() !== "" && image.status !== Image.Error
    readonly property var colors: Theme.coverPalette[root.paletteIndex(root.title)]
    readonly property real stackInset: root.stacked ? 2 * Theme.coverStackOffset : 0

    implicitWidth: Theme.coverTileMinWidth
    implicitHeight: implicitWidth * Theme.coverAspect

    function paletteIndex(text: string): int {
        let hash = 0
        for (let i = 0; i < text.length; ++i)
            hash = (hash * 31 + text.charCodeAt(i)) % 100003
        return hash % Theme.coverPalette.length
    }

    // Generated cover: gradient with title and author.
    component PlaceholderFace: Rectangle {
        id: face

        required property string title
        required property string author
        required property var colors
        property real titleInset: 0

        gradient: Gradient {
            GradientStop { position: 0; color: face.colors[0] }
            GradientStop { position: 1; color: face.colors[1] }
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spacingS
            anchors.topMargin: Theme.spacingS + face.titleInset
            // Keep the top right corner free for the format badges.
            anchors.rightMargin: Theme.badgeSize + Theme.spacingS
            // Long words shrink the font instead of being broken apart.
            height: parent.height * Theme.coverTitleHeight
            // Thumbnails (e.g. in the import queue) are too small for text.
            visible: face.width >= Theme.coverTextMinWidth
            text: face.title
            wrapMode: Text.WordWrap
            fontSizeMode: Text.Fit
            minimumPixelSize: Theme.fontSmall
            verticalAlignment: Text.AlignTop
            color: Theme.coverPlaceholderText
            font.family: Theme.fontFamilySerif
            font.pixelSize: Math.max(Theme.fontSmall, Math.round(face.width * Theme.coverTitleScale))
            font.weight: Font.Bold
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.spacingS
            visible: face.width >= Theme.coverTextMinWidth
            text: face.author
            elide: Text.ElideRight
            opacity: Theme.coverAuthorOpacity
            color: Theme.coverPlaceholderText
            font.pixelSize: Theme.fontSmall
        }
    }

    // Sheets behind a series stack.
    Rectangle {
        visible: root.stacked
        x: frame.x + 2 * Theme.coverStackOffset
        y: frame.y - 2 * Theme.coverStackOffset
        width: frame.width
        height: frame.height
        radius: Theme.radiusS
        color: Theme.coverStackBack
    }
    Rectangle {
        visible: root.stacked
        x: frame.x + Theme.coverStackOffset
        y: frame.y - Theme.coverStackOffset
        width: frame.width
        height: frame.height
        radius: Theme.radiusS
        color: Theme.coverStackMiddle
    }

    Item {
        id: frame

        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: root.stackInset
        anchors.topMargin: root.stackInset

        // Everything inside is rendered into a layer and masked to the rounded
        // frame. The item stays visible (the effect is its layer.effect), so
        // positioners inside, e.g. the badge column of CoverTile, keep updating.
        Item {
            id: content

            anchors.fill: parent
            layer.enabled: true
            layer.effect: MultiEffect {
                maskEnabled: true
                maskSource: mask
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }

            // --- placeholder -------------------------------------------------
            // Fill above and below a square placeholder: a clearly darker
            // version of its gradient, so the square stands out like a real
            // cover does against its blurred fill.
            Rectangle {
                anchors.fill: parent
                visible: !root.hasImage && root.coverIsSquare
                gradient: Gradient {
                    GradientStop { position: 0; color: Qt.darker(root.colors[0], Theme.coverFillDarkening) }
                    GradientStop { position: 1; color: Qt.darker(root.colors[1], Theme.coverFillDarkening) }
                }
            }
            PlaceholderFace {
                visible: !root.hasImage
                width: parent.width
                height: root.coverIsSquare ? parent.width : parent.height
                anchors.verticalCenter: parent.verticalCenter
                title: root.title
                author: root.author
                colors: root.colors
                // A square face starts below the top of the frame anyway.
                titleInset: root.coverIsSquare ? 0 : root.titleInset
            }

            // --- square image: blurred fill ----------------------------------
            // Larger than the frame, so the blur does not fade out at the edges.
            Image {
                id: blurSource

                anchors.fill: parent
                anchors.margins: -Theme.coverBlurMax
                visible: false
                source: root.hasImage && root.coverIsSquare ? root.coverUrl : ""
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                sourceSize.width: Theme.coverDecodeWidth / 4
            }
            MultiEffect {
                anchors.fill: blurSource
                visible: root.hasImage && root.coverIsSquare
                source: blurSource
                autoPaddingEnabled: false
                blurEnabled: true
                blur: 1.0
                blurMax: Theme.coverBlurMax
                brightness: Theme.coverBlurBrightness
            }

            // --- image -------------------------------------------------------
            Image {
                id: image

                width: parent.width
                height: root.coverIsSquare ? parent.width : parent.height
                anchors.verticalCenter: parent.verticalCenter
                visible: root.hasImage
                source: root.coverUrl
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                sourceSize.width: Theme.coverDecodeWidth
            }

            Item {
                id: overlayItem

                anchors.fill: parent
            }
        }

        Item {
            id: mask

            anchors.fill: parent
            visible: false
            layer.enabled: true

            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusS
                color: Theme.textPrimary // any opaque color
            }
        }
    }
}

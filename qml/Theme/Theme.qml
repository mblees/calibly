pragma Singleton
import QtQuick

// Design tokens of calibly.
// Components take colors, spacing, radii and font sizes only from here and
// never hard-code values (UI Development Plan, rule 5). A light theme later
// only needs a second set of these values.
QtObject {
    // Surfaces
    readonly property color background: "#16171b"
    readonly property color sidebar: "#1b1c21"
    readonly property color surface: "#212228"
    readonly property color surfaceRaised: "#2a2b32"
    readonly property color surfaceSelected: "#2b2c33"
    readonly property color border: "#2c2d34"
    readonly property color divider: "#111215"

    // Text
    readonly property color textPrimary: "#ececef"
    readonly property color textSecondary: "#b8b8c0"
    readonly property color textMuted: "#8f8f99"
    readonly property color textDisabled: "#62626b"

    // Semantic colors: blue = reading / ebook, orange = listening / audiobook
    // and accent, green = finished.
    readonly property color accent: "#ff8a3d"
    readonly property color accentForeground: "#1a0d03"
    readonly property color read: "#6ea8ff"
    readonly property color readForeground: "#06152b"
    readonly property color listen: accent
    readonly property color listenForeground: accentForeground
    readonly property color finished: "#5fcf80"
    readonly property color finishedForeground: "#062010"
    readonly property color danger: "#e05a5a"
    readonly property color shadow: "#80000000"
    readonly property color scrim: "#99000000"      // dims the window behind drawers
    readonly property color rating: "#f2c14e"

    // Spacing
    readonly property int spacingXxs: 2
    readonly property int spacingXs: 4
    readonly property int spacingS: 8
    readonly property int spacingM: 12
    readonly property int spacingL: 16
    readonly property int spacingXl: 24
    readonly property int spacingXxl: 32

    // Corner radii
    readonly property int radiusS: 4
    readonly property int radiusM: 8
    readonly property int radiusL: 12

    // Font pixel sizes
    readonly property int fontSmall: 11
    readonly property int fontBody: 13
    readonly property int fontLarge: 15
    readonly property int fontTitle: 20
    readonly property int fontDisplay: 28
    readonly property real lineHeightText: 1.4 // running text such as descriptions

    // Font families
    readonly property string fontFamilySerif: "Georgia"

    // Sizes
    readonly property int iconSize: 16
    readonly property int iconSizeSmall: 11
    readonly property int iconSizeLarge: 48
    readonly property int controlHeight: 34
    readonly property int chipHeight: 30
    readonly property int searchFieldWidth: 380
    readonly property int menuMinWidth: 200
    readonly property int sliderWidth: 110
    readonly property int sliderHandleSize: 14
    readonly property int drawerWidth: 580
    readonly property int detailCoverWidth: 150
    readonly property int tabHeight: 36
    readonly property int ratingStarSize: 14
    readonly property int formatCardWidth: 260
    readonly property real formatBorderOpacity: 0.4 // colored border of format cards
    readonly property int sidebarWidth: 220
    readonly property int borderWidth: 1
    readonly property int focusWidth: 2
    readonly property int badgeSize: 20
    readonly property real badgeIconStrokeWidth: 2.4 // thicker lines, so the small icons stay readable
    readonly property real disabledOpacity: 0.4
    readonly property int progressHeight: 3

    // Covers
    readonly property real coverAspect: 1.5        // height / width of the 2:3 frame
    readonly property int coverTileMinWidth: 150   // grid columns are derived from this
    readonly property int coverTileSmallest: 110   // range of the cover size slider
    readonly property int coverTileLargest: 240
    readonly property int coverTileTextHeight: 44  // title + author below a cover
    readonly property int coverDecodeWidth: 400    // decode size of cover images (memory)
    readonly property int coverStackOffset: 3      // offset of the sheets behind a series stack
    readonly property int coverBlurMax: 48
    readonly property real coverFillDarkening: 2.2  // Qt.darker() factor for the fill around square placeholders
    readonly property real coverBlurBrightness: -0.2 // darkening of the blurred fill around square images
    readonly property real coverTitleScale: 0.1      // placeholder title font size relative to the cover width
    readonly property real coverTitleHeight: 0.6     // share of the cover height available to the placeholder title
    readonly property real coverAuthorOpacity: 0.8   // placeholder author text
    readonly property color coverOverlay: "#b30a0a0c"     // background of badges on covers
    readonly property color coverProgressTrack: "#80000000"
    readonly property color coverStackBack: "#3a3b44"
    readonly property color coverStackMiddle: "#4a4b55"
    readonly property color coverPlaceholderText: "#ffffff"
    // Gradients (top, bottom) for works without a cover image.
    readonly property var coverPalette: [
        ["#274b6d", "#0f2236"], ["#9b3d2a", "#4a160c"], ["#3f6b4a", "#16301d"],
        ["#c49a3a", "#6e4c10"], ["#5c3f78", "#25133a"], ["#2f6f73", "#0d3234"],
        ["#a8475f", "#4d1424"], ["#575d66", "#1f2328"], ["#c46a2b", "#5d2a07"],
        ["#3a4f9c", "#141e4d"], ["#7a6a50", "#352b1b"], ["#4d7e8f", "#173743"]
    ]

    // Motion
    readonly property int durationFast: 120
    readonly property int durationDrawer: 220
    readonly property real heroBlurBrightness: -0.1  // darkening of the blurred cover behind the detail header
    readonly property real heroOverlayOpacity: 0.5   // background-colored veil over it, keeps the text readable
}

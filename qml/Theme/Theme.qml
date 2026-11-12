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
    readonly property color dialog: "#1e1f24"
    readonly property color inputBackground: "#15161a"
    readonly property color inputBorder: "#34353c"
    readonly property color playerBar: "#0f1013"
    readonly property color playerPanel: "#8c0f1013"   // chapter list over the blurred cover
    readonly property color playButton: "#ffffff"
    readonly property color playButtonForeground: "#111111"
    readonly property color seekTrack: "#2effffff"
    readonly property color hoverTint: "#14ffffff"      // hover on translucent backgrounds

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
    readonly property color dangerForeground: "#ffffff"
    readonly property color shadow: "#80000000"
    readonly property color scrim: "#99000000"      // dims the window behind drawers
    readonly property color rating: "#f2c14e"
    readonly property color shared: finished        // marks fields that apply to all formats
    readonly property real focusRingOpacity: 0.15   // accent glow around a focused input
    readonly property real currentRowOpacity: 0.16  // accent background of the current chapter

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
    readonly property int focusRingWidth: 3
    readonly property int textAreaHeight: 120
    readonly property int tagHeight: 22
    readonly property int dialogWidth: 960
    readonly property int dialogHeight: 680
    readonly property int editorCoverWidth: 160
    readonly property int formColumnSpacing: 14
    readonly property int popupMaxHeight: 320
    readonly property int playerBarHeight: 68
    readonly property int playerBarCoverSize: 46
    readonly property int playerBarInfoWidth: 220
    readonly property int playButtonSize: 38
    readonly property int playButtonSizeLarge: 64
    readonly property int iconSizePlayer: 22
    readonly property int playerCoverSize: 290
    readonly property int playerColumnWidth: 580
    readonly property int seekHeight: 5
    readonly property int seekHeightLarge: 6
    readonly property int seekHandleSize: 14
    readonly property int chapterRowHeight: 36
    readonly property int chapterNumberWidth: 28
    readonly property real playerBlurBrightness: -0.12 // darkening of the blurred cover behind the player
    readonly property real playerBlurSaturation: 0.4
    readonly property int playerBlurSourceSize: 12      // decode size of the cover behind the player
    readonly property real formatBorderOpacity: 0.4 // colored border of format cards
    readonly property int sidebarWidth: 220
    readonly property int borderWidth: 1
    readonly property int focusWidth: 2
    readonly property int badgeSize: 20
    readonly property real badgeIconStrokeWidth: 2.4 // thicker lines, so the small icons stay readable
    readonly property real disabledOpacity: 0.4
    readonly property int progressHeight: 3
    readonly property int continueCardWidth: 300
    readonly property int continueCoverWidth: 46  // cover in a "Continue" card (2:3)
    readonly property int confirmDialogWidth: 440
    readonly property int selectCircleSize: 22    // selection circle on a cover
    readonly property int importQueueWidth: 340
    readonly property int importQueueMaxHeight: 320
    readonly property int importThumbWidth: 26    // cover thumbnail in the import queue (2:3)
    readonly property int dropGlowWidth: 6        // accent glow around the cover files are dropped on
    readonly property real dropGlowOpacity: 0.35
    readonly property int dropScrollMargin: 56    // dragging this close to the grid edge scrolls it
    readonly property int dropScrollStep: 14      // pixels per scroll tick while dragging

    // Covers
    readonly property real coverAspect: 1.5        // height / width of the 2:3 frame
    readonly property int coverTileMinWidth: 150   // grid columns are derived from this
    readonly property int coverTileSmallest: 110   // range of the cover size slider
    readonly property int coverTileLargest: 240
    readonly property int coverTileTextHeight: 44  // title + author below a cover
    readonly property int coverDecodeWidth: 400    // decode size of cover images (memory)
    readonly property int coverTextMinWidth: 60    // smaller placeholders show no title and author
    readonly property int coverStackOffset: 3      // offset of the sheets behind a series stack
    readonly property int coverBlurMax: 48
    readonly property real coverFillDarkening: 2.2  // Qt.darker() factor for the fill around square placeholders
    readonly property real coverBlurBrightness: -0.2 // darkening of the blurred fill around square images
    readonly property real coverTitleScale: 0.1      // placeholder title font size relative to the cover width
    readonly property real coverTitleHeight: 0.6     // share of the cover height available to the placeholder title
    readonly property real coverAuthorOpacity: 0.8   // placeholder author text
    readonly property color coverOverlay: "#b30a0a0c"     // background of badges on covers
    readonly property color coverHoverScrim: "#80000000"  // darkens a cover behind its hover actions
    readonly property int coverActionWidth: 120           // "Lesen" / "Hören" on a hovered cover
    readonly property real missingSaturation: -1.0        // cover of a work whose file is missing: grey …
    readonly property real missingBrightness: -0.12       // … and a little darker
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

    // Reader (phase 10). Page, text and link colors of the page colors of
    // ReaderSettings.pageColor; images keep their own colors.
    readonly property var readerPageColors: ({
        "weiss": { "page": "#fbfaf7", "text": "#1f1f22", "link": "#2c5fb8" },
        "sepia": { "page": "#f4ecd8", "text": "#3b2f22", "link": "#8a4a12" },
        "grau": { "page": "#3a3b40", "text": "#e3e3e6", "link": "#9cc2ff" },
        "nacht": { "page": "#121316", "text": "#d9d9de", "link": "#6ea8ff" }
    })
    readonly property color readerBar: "#f00f1013"      // bars over the page in fullscreen
    readonly property color readerHint: "#eb2a2b32"     // hint when entering fullscreen
    readonly property int readerBarHeight: 48
    readonly property int readerBottomBarHeight: 56
    readonly property int readerProgressHeight: 2       // fine progress line in fullscreen
    readonly property int readerHintHeight: 32
    readonly property int readerInfoWidth: 170          // "Seite 3 von 96 · 3 %" in the bottom bar
    readonly property int readerChapterWidth: 140       // chapter in the bottom bar
    readonly property int readerNarrowWidth: 900        // narrower windows hide the contents at first
    readonly property int contentsWidth: 240
    readonly property int contentsRowHeight: 32
    readonly property int textSettingsWidth: 360
    readonly property int textSettingsLabelWidth: 118
    readonly property int fontStepButtonWidth: 52
    readonly property int fontStepDotSize: 7
    readonly property int themeSwatchHeight: 74
    readonly property int pageColorSwatchHeight: 44
    readonly property int themeSwatchFontSize: 24
    readonly property int fontStepLargeSize: 22
    readonly property int toggleWidth: 40
    readonly property int toggleHeight: 22
    readonly property int toggleKnobSize: 16
    readonly property color toggleOff: "#4a4b55"
    readonly property color toggleKnob: "#ffffff"
    readonly property int readerBarHideDelay: 3000      // bars over the page hide after this
    readonly property int readerHintDuration: 2000

    // Motion
    readonly property int durationFast: 120
    readonly property int durationDrawer: 220
    readonly property int messageDuration: 6000 // how long short messages stay visible
    readonly property int spinnerDuration: 900  // one turn of the busy spinner
    readonly property int dropScrollInterval: 16
    readonly property real heroBlurBrightness: -0.1  // darkening of the blurred cover behind the detail header
    readonly property real heroOverlayOpacity: 0.5   // background-colored veil over it, keeps the text readable
}

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

    // Sizes
    readonly property int iconSize: 16
    readonly property int iconSizeLarge: 48
    readonly property int controlHeight: 34
    readonly property int sidebarWidth: 220
    readonly property int borderWidth: 1
    readonly property int focusWidth: 2

    // Motion
    readonly property int durationFast: 120
}

// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma Singleton
import QtQuick

// SVG path data for all icons, drawn on a 24 × 24 grid as line icons.
// Used by Icon.qml. To add an icon, add one entry to "paths".
//
// Path data adapted from Feather Icons (https://feathericons.com),
// Copyright (c) 2013-2017 Cole Bemis, MIT License.
QtObject {
    // Icons that are filled instead of stroked.
    readonly property var filled: ["play", "pause", "star"]

    readonly property var paths: ({
        "book": "M4 19.5A2.5 2.5 0 0 1 6.5 17H20 M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z",
        "headphones": "M3 18v-6a9 9 0 0 1 18 0v6 M21 19a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3z M3 19a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a2 2 0 0 0-2-2H3z",
        "library": "M3 3h7v7H3z M14 3h7v7h-7z M14 14h7v7h-7z M3 14h7v7H3z",
        "clock": "M2 12a10 10 0 1 0 20 0a10 10 0 1 0-20 0 M12 6v6l4 2",
        "layers": "M12 2L2 7l10 5 10-5-10-5z M2 17l10 5 10-5 M2 12l10 5 10-5",
        "user": "M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2 M8 7a4 4 0 1 0 8 0a4 4 0 1 0-8 0",
        "mic": "M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z M19 10v2a7 7 0 0 1-14 0v-2 M12 19v4 M8 23h8",
        "search": "M3 11a8 8 0 1 0 16 0a8 8 0 1 0-16 0 M21 21l-4.35-4.35",
        "zoomIn": "M3 11a8 8 0 1 0 16 0a8 8 0 1 0-16 0 M21 21l-4.35-4.35 M11 8v6 M8 11h6",
        "image": "M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z M7 8.5a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0-3 0 M21 15l-5-5L5 21",
        "clipboard": "M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2 M8 2h8v4H8z",
        "chevronUp": "M18 15l-6-6-6 6",
        "plus": "M12 5v14 M5 12h14",
        "x": "M18 6L6 18 M6 6l12 12",
        "skipBack": "M3 12a9 9 0 1 0 3-6.7L3 8 M3 3v5h5",
        "skipForward": "M21 12a9 9 0 1 1-3-6.7L21 8 M21 3v5h-5",
        "previous": "M19 20L9 12l10-8z M5 19V5",
        "next": "M5 4l10 8-10 8z M19 5v14",
        "moon": "M21 12.8A9 9 0 1 1 11.2 3 7 7 0 0 0 21 12.8z",
        "expand": "M15 3h6v6 M9 21H3v-6 M21 3l-7 7 M3 21l7-7",
        "shrink": "M4 14h6v6 M20 10h-6V4 M14 10l7-7 M3 21l7-7",
        "minus": "M5 12h14",
        "externalLink": "M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6 M15 3h6v6 M10 14L21 3",
        "link": "M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71 M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71",
        "edit": "M12 20h9 M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z",
        "list": "M8 6h13 M8 12h13 M8 18h13 M3 6h.01 M3 12h.01 M3 18h.01",
        "chevronLeft": "M15 18l-6-6 6-6",
        "chevronRight": "M9 18l6-6-6-6",
        "chevronDown": "M6 9l6 6 6-6",
        "check": "M20 6L9 17l-5-5",
        "trash": "M3 6h18 M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6 M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2 M10 11v6 M14 11v6",
        "folder": "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z",
        "more": "M11 12a1 1 0 1 0 2 0a1 1 0 1 0-2 0 M18 12a1 1 0 1 0 2 0a1 1 0 1 0-2 0 M4 12a1 1 0 1 0 2 0a1 1 0 1 0-2 0",
        "reset": "M1 4v6h6 M3.51 15a9 9 0 1 0 2.13-9.36L1 10",
        "split": "M16 3h5v5 M8 3H3v5 M21 3l-7 7 M3 3l7 7 M12 14v7",
        "upload": "M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4 M17 8l-5-5-5 5 M12 3v12",
        "download": "M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4 M7 10l5 5 5-5 M12 15V3",
        "alert": "M2 12a10 10 0 1 0 20 0a10 10 0 1 0-20 0 M12 8v4 M12 16h.01",
        "sort": "M3 6h18 M6 12h12 M10 18h4",
        "gridSmall": "M3 3h4v4H3z M10 3h4v4h-4z M17 3h4v4h-4z M3 10h4v4H3z M10 10h4v4h-4z M17 10h4v4h-4z M3 17h4v4H3z M10 17h4v4h-4z M17 17h4v4h-4z",
        "play": "M5 3l14 9-14 9V3z",
        "star": "M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z",
        "pause": "M6 4h4v16H6z M14 4h4v16h-4z",
        "settings": "M9 12a3 3 0 1 0 6 0a3 3 0 1 0-6 0 M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"
    })
}

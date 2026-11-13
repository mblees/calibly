// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls
import Calibly.Components
import Calibly.Controllers

// View G of the plan: the ebook reader (mockup calibly-reader-hybrid.html).
// Main.qml lays it over the content area while ReaderController has a book
// open: the sidebar and the player bar stay, the contents are a column on
// the right. The fullscreen button or F11 put the window into fullscreen
// with only the page and a fine progress line; moving the mouse or a click
// in the middle of the page shows the bars over the page for 3 seconds.
// Esc leaves fullscreen, the next Esc closes the book.
// Turning pages: ← → PgUp PgDn Space Home End, the mouse wheel and a click
// on the left or right third of the page; Ctrl + / Ctrl − change the font
// size (PDF: the zoom).
//
// Properties:
//   backText    label of the way back, e.g. "Bibliothek"
//   fullscreen  the reader fills the screen (read-only: use toggleFullscreen());
//               Main.qml hides the sidebar and the player bar meanwhile
// Functions:
//   handleKey(event)    the keys of the reader; Main.qml passes on the keys
//                       no control used while a book is open
//   toggleFullscreen()
Item {
    id: root

    property string backText: qsTr("Bibliothek")
    readonly property bool fullscreen: root.inFullscreen

    property bool inFullscreen: false
    property int windowVisibility: Window.Windowed  // to return to after fullscreen
    property bool contentsShown: false
    property bool contentsShownInWindow: false
    property bool barsShown: true
    property bool overLink: false
    property real wheelDelta: 0

    readonly property bool isPdf: ReaderController.type === ReaderController.Pdf
    readonly property bool hasError: ReaderController.error !== ReaderController.NoError
    readonly property var colors: Theme.readerPageColors[ReaderSettings.pageColor] ?? Theme.readerPageColors["nacht"]
    // The bars stay while the user works with them.
    readonly property bool barsBusy: settingsPopup.visible || topHover.hovered || bottomHover.hovered
                                     || (root.inFullscreen && root.contentsShown) || root.hasError
    // Space next to the page in the window for the ‹ › buttons.
    readonly property real sideSpace: root.inFullscreen ? 0 : Theme.controlHeight + Theme.spacingS

    // "Seite 3 von 96 · 3 %"; while the pages are being counted only "3 %".
    function pageText(progress: real): string {
        const percent = qsTr("%1 %").arg(Math.round(progress * 100))
        const count = ReaderController.pageCount
        const page = ReaderController.page
        if (count <= 0 || page < 0)
            return percent
        if (ReaderController.lastPage > page)
            return qsTr("Seite %1–%2 von %3 · %4").arg(page + 1).arg(ReaderController.lastPage + 1)
                       .arg(count).arg(percent)
        return qsTr("Seite %1 von %2 · %3").arg(page + 1).arg(count).arg(percent)
    }

    function toggleFullscreen() {
        const window = root.Window.window
        if (!root.inFullscreen) {
            root.contentsShownInWindow = root.contentsShown
            root.contentsShown = false
            root.inFullscreen = true
            if (window) {
                root.windowVisibility = window.visibility
                window.showFullScreen()
            }
            hint.show()
            root.showBars()
        } else {
            root.inFullscreen = false
            root.contentsShown = root.contentsShownInWindow
            root.barsShown = true
            if (window) {
                if (root.windowVisibility === Window.Maximized)
                    window.showMaximized()
                else
                    window.showNormal()
            }
        }
    }

    // In fullscreen: shows the bars over the page; they hide again a moment
    // after the mouse stopped.
    function showBars() {
        root.barsShown = true
        if (root.inFullscreen)
            hideTimer.restart()
    }

    function handleKey(event: var) {
        const control = (event.modifiers & Qt.ControlModifier) !== 0
        switch (event.key) {
        case Qt.Key_Left:
        case Qt.Key_PageUp:
        case Qt.Key_Up:
            ReaderController.previousPage()
            break
        case Qt.Key_Right:
        case Qt.Key_PageDown:
        case Qt.Key_Down:
        case Qt.Key_Space:
            ReaderController.nextPage()
            break
        case Qt.Key_Home:
            ReaderController.goToStart()
            break
        case Qt.Key_End:
            ReaderController.goToEnd()
            break
        case Qt.Key_Plus:
        case Qt.Key_Equal:
            if (!control)
                return
            if (root.isPdf)
                ReaderController.zoomIn()
            else
                ReaderSettings.larger()
            break
        case Qt.Key_Minus:
            if (!control)
                return
            if (root.isPdf)
                ReaderController.zoomOut()
            else
                ReaderSettings.smaller()
            break
        case Qt.Key_F11:
            root.toggleFullscreen()
            break
        case Qt.Key_Escape:
            if (root.inFullscreen)
                root.toggleFullscreen()
            else
                ReaderController.close()
            break
        default:
            return
        }
        event.accepted = true
    }

    visible: ReaderController.isOpen
    clip: true

    Keys.onPressed: function(event) {
        root.handleKey(event)
    }

    // A new book: the contents column only where there is room for it, and
    // the keys go to the reader. A closed book leaves fullscreen.
    Connections {
        target: ReaderController

        function onOpenChanged() {
            if (ReaderController.isOpen) {
                root.contentsShown = !root.inFullscreen && root.width >= Theme.readerNarrowWidth
                root.contentsShownInWindow = root.width >= Theme.readerNarrowWidth
                root.barsShown = true
                root.forceActiveFocus()
            } else {
                settingsPopup.close()
                if (root.inFullscreen)
                    root.toggleFullscreen()
            }
        }
    }

    Binding {
        target: ReaderController
        property: "viewSize"
        value: Qt.size(flick.width, flick.height)
    }
    Binding {
        target: ReaderController
        property: "twoPagesWanted"
        value: root.inFullscreen && ReaderSettings.twoPages
    }
    Binding {
        target: ReaderController
        property: "pageColor"
        value: root.colors.page
    }
    Binding {
        target: ReaderController
        property: "textColor"
        value: root.colors.text
    }
    Binding {
        target: ReaderController
        property: "linkColor"
        value: root.colors.link
    }

    Timer {
        id: hideTimer

        interval: Theme.readerBarHideDelay
        onTriggered: {
            if (root.barsBusy)
                hideTimer.restart()
            else
                root.barsShown = false
        }
    }

    // --- the page ---------------------------------------------------------------
    Rectangle {
        id: pageArea

        anchors.left: parent.left
        anchors.right: root.contentsShown && !root.inFullscreen ? contents.left : parent.right
        anchors.top: root.inFullscreen ? parent.top : topBar.bottom
        anchors.bottom: root.inFullscreen ? parent.bottom : bottomBar.top
        color: root.isPdf || root.hasError ? Theme.background : root.colors.page

        Flickable {
            id: flick

            anchors.fill: parent
            anchors.leftMargin: root.sideSpace
            anchors.rightMargin: root.sideSpace
            anchors.topMargin: root.inFullscreen ? Theme.spacingXl : 0
            anchors.bottomMargin: root.inFullscreen ? Theme.spacingXl : 0
            visible: !root.hasError
            contentWidth: pageHolder.width
            contentHeight: pageHolder.height
            interactive: contentWidth > width || contentHeight > height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Item {
                id: pageHolder

                width: Math.max(flick.width, page.width)
                height: Math.max(flick.height, page.height)

                BookPage {
                    id: page

                    anchors.centerIn: parent
                    width: ReaderController.pageItemSize.width
                    height: ReaderController.pageItemSize.height
                }

                // Links, the click zones and the mouse wheel. In fullscreen
                // a mouse movement shows the bars.
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: root.overLink ? Qt.PointingHandCursor
                                               : root.inFullscreen && !root.barsShown ? Qt.BlankCursor
                                                                                       : Qt.ArrowCursor

                    onPositionChanged: function(mouse) {
                        const point = mapToItem(page, mouse.x, mouse.y)
                        root.overLink = ReaderController.linkAt(point.x, point.y) !== ""
                        if (root.inFullscreen)
                            root.showBars()
                    }
                    onExited: root.overLink = false

                    onClicked: function(mouse) {
                        const point = mapToItem(page, mouse.x, mouse.y)
                        const href = ReaderController.linkAt(point.x, point.y)
                        if (href !== "") {
                            ReaderController.followLink(href)
                            return
                        }
                        const x = mapToItem(pageArea, mouse.x, mouse.y).x
                        if (x < pageArea.width / 3) {
                            ReaderController.previousPage()
                        } else if (x > pageArea.width * 2 / 3) {
                            ReaderController.nextPage()
                        } else if (root.inFullscreen) {
                            if (root.barsShown) {
                                hideTimer.stop()
                                root.barsShown = false
                            } else {
                                root.showBars()
                            }
                        }
                    }

                    // One page per notch; a zoomed PDF scrolls instead.
                    onWheel: function(wheel) {
                        if (flick.interactive) {
                            wheel.accepted = false
                            return
                        }
                        root.wheelDelta += wheel.angleDelta.y
                        if (root.wheelDelta <= -120) {
                            root.wheelDelta = 0
                            ReaderController.nextPage()
                        } else if (root.wheelDelta >= 120) {
                            root.wheelDelta = 0
                            ReaderController.previousPage()
                        }
                    }
                }
            }
        }

        IconButton {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingXs
            anchors.verticalCenter: parent.verticalCenter
            visible: !root.inFullscreen && !root.hasError
            enabled: !ReaderController.atStart
            opacity: enabled ? 1 : Theme.disabledOpacity
            iconName: "chevronLeft"
            iconColor: Theme.textMuted
            text: qsTr("Vorige Seite")
            onClicked: ReaderController.previousPage()
        }
        IconButton {
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingXs
            anchors.verticalCenter: parent.verticalCenter
            visible: !root.inFullscreen && !root.hasError
            enabled: !ReaderController.atEnd
            opacity: enabled ? 1 : Theme.disabledOpacity
            iconName: "chevronRight"
            iconColor: Theme.textMuted
            text: qsTr("Nächste Seite")
            onClicked: ReaderController.nextPage()
        }

        // --- missing or unreadable file --------------------------------------
        Column {
            anchors.centerIn: parent
            width: Math.min(parent.width - 2 * Theme.spacingXxl, Theme.confirmDialogWidth)
            visible: root.hasError
            spacing: Theme.spacingM

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "alert"
                size: Theme.iconSizeLarge
                color: Theme.textMuted
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: ReaderController.error === ReaderController.FileMissing
                      ? qsTr("Datei fehlt") : qsTr("Dieses Buch lässt sich hier nicht anzeigen")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: ReaderController.error === ReaderController.FileMissing
                      ? qsTr("Die Datei dieses Buchs wurde nicht gefunden. Vielleicht wurde sie verschoben oder gelöscht.")
                      : qsTr("Die Datei ist beschädigt oder kopiergeschützt (DRM). Ein anderes Programm kann sie vielleicht öffnen.")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontBody
                lineHeight: Theme.lineHeightText
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                topPadding: Theme.spacingS
                spacing: Theme.spacingS

                AppButton {
                    visible: ReaderController.error === ReaderController.Unreadable
                    variant: AppButton.Read
                    iconName: "externalLink"
                    text: qsTr("Mit Standardprogramm öffnen")
                    onClicked: {
                        if (ReaderController.openExternally())
                            ReaderController.close()
                    }
                }
                AppButton {
                    text: qsTr("Zurück")
                    onClicked: ReaderController.close()
                }
            }
        }
    }

    // --- contents ------------------------------------------------------------------
    // A column in the window; over the page between the bars in fullscreen.
    TableOfContents {
        id: contents

        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.bottom: bottomBar.top
        visible: root.contentsShown && !root.hasError && (!root.inFullscreen || root.barsShown)
        entries: ReaderController.contents
        currentIndex: ReaderController.contentsIndex
        onEntryActivated: function(index) {
            ReaderController.goToContents(index)
        }
    }

    // --- bars ----------------------------------------------------------------------
    ReaderBar {
        id: topBar

        width: parent.width
        height: Theme.readerBarHeight
        opacity: !root.inFullscreen || root.barsShown ? 1 : 0
        visible: opacity > 0
        backText: root.backText
        title: ReaderController.title
        // A volume of a combined work: "Band 2 von 3 · Autor".
        author: ReaderController.volumeCount > 1
                ? [qsTr("Band %1 von %2").arg(ReaderController.volumeNumber).arg(ReaderController.volumeCount),
                   ReaderController.author].filter(part => part !== "").join(" · ")
                : ReaderController.author
        chapterTitle: ReaderController.chapterTitle
        pdf: root.isPdf
        fullscreen: root.inFullscreen
        contentsShown: root.contentsShown
        settingsOpen: settingsPopup.visible

        onBackRequested: ReaderController.close()
        onContentsToggled: root.contentsShown = !root.contentsShown
        onSettingsToggled: settingsPopup.visible ? settingsPopup.close() : settingsPopup.open()
        onFullscreenToggled: root.toggleFullscreen()

        Behavior on opacity {
            NumberAnimation { duration: Theme.durationDrawer }
        }

        HoverHandler {
            id: topHover
        }
    }

    // Chapter, slider and page; in fullscreen over the page like the top bar.
    Rectangle {
        id: bottomBar

        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.readerBottomBarHeight
        opacity: !root.inFullscreen || root.barsShown ? 1 : 0
        visible: opacity > 0 && !root.hasError
        color: root.inFullscreen ? Theme.readerBar : Theme.background

        Behavior on opacity {
            NumberAnimation { duration: Theme.durationDrawer }
        }

        HoverHandler {
            id: bottomHover
        }

        Rectangle {
            width: parent.width
            height: Theme.borderWidth
            color: Theme.border
        }

        Text {
            id: chapterText

            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingXl
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.readerChapterWidth
            elide: Text.ElideRight
            text: ReaderController.chapterTitle
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }

        SeekBar {
            id: seekBar

            anchors.left: chapterText.right
            anchors.leftMargin: Theme.spacingL
            anchors.right: pageInfo.left
            anchors.rightMargin: Theme.spacingL
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.read
            value: ReaderController.progress
            Accessible.name: qsTr("Stelle im Buch")
            onSeekRequested: function(value) {
                ReaderController.seek(value)
            }
        }

        Text {
            id: pageInfo

            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingXl
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.readerInfoWidth
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideLeft
            text: seekBar.pressed ? qsTr("%1 %").arg(Math.round(seekBar.dragValue * 100))
                                  : root.pageText(ReaderController.progress)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
            font.features: { "tnum": 1 }
        }
    }

    // --- fullscreen: fine progress line and hint ---------------------------------------
    ProgressLine {
        id: progressLine

        anchors.bottom: parent.bottom
        width: parent.width
        height: root.inFullscreen ? Theme.readerProgressHeight : 0
        visible: root.inFullscreen && !root.barsShown
        rounded: false
        value: ReaderController.progress
        color: Theme.read
        trackColor: Theme.surfaceRaised
    }
    Text {
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingL + Theme.spacingXs
        anchors.bottom: progressLine.top
        anchors.bottomMargin: Theme.spacingS
        visible: root.inFullscreen && !root.barsShown && !root.hasError
        text: qsTr("%1 %").arg(Math.round(ReaderController.progress * 100))
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall + 1
        font.features: { "tnum": 1 }
    }

    Rectangle {
        id: hint

        function show() {
            hint.opacity = 1
            hintTimer.restart()
        }

        anchors.horizontalCenter: parent.horizontalCenter
        y: Theme.readerBarHeight + Theme.spacingL
        width: hintText.implicitWidth + 2 * Theme.spacingL
        height: Theme.readerHintHeight
        radius: height / 2
        opacity: 0
        visible: opacity > 0
        color: Theme.readerHint

        Behavior on opacity {
            NumberAnimation { duration: Theme.durationDrawer }
        }

        Text {
            id: hintText

            anchors.centerIn: parent
            text: qsTr("Leisten erscheinen bei Mausbewegung · Esc verlässt das Vollbild")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall + 1
        }

        Timer {
            id: hintTimer

            interval: Theme.readerHintDuration
            onTriggered: hint.opacity = 0
        }
    }

    // --- text settings ("Aa"), for PDFs the zoom -------------------------------------------
    T.Popup {
        id: settingsPopup

        parent: topBar.settingsButton
        x: parent ? parent.width - width : 0
        y: parent ? parent.height + Theme.spacingS : 0
        width: Theme.textSettingsWidth
        height: Math.min(settings.implicitHeight, root.height - Theme.readerBarHeight - 2 * Theme.spacingL)
        padding: 0
        margins: Theme.spacingS
        focus: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent

        // Back to the page, so the keys turn pages again.
        onClosed: root.forceActiveFocus()

        background: null

        contentItem: TextSettings {
            id: settings

            pdf: root.isPdf
            fontSizeStep: ReaderSettings.fontSizeStep
            fontSizeSteps: ReaderSettings.fontSizeSteps
            theme: ReaderSettings.theme
            themes: ReaderSettings.themes.map(id => Object.assign({ "id": id }, ReaderSettings.themeInfo(id)))
            pageColor: ReaderSettings.pageColor
            fontFamily: ReaderSettings.fontFamily
            fontFamilies: ReaderSettings.fontFamilies
            bold: ReaderSettings.bold
            lineHeight: ReaderSettings.lineHeight
            letterSpacing: ReaderSettings.letterSpacing
            wordSpacing: ReaderSettings.wordSpacing
            margin: ReaderSettings.margin
            justify: ReaderSettings.justify
            twoPages: ReaderSettings.twoPages
            zoomMode: ReaderController.zoomMode
            zoom: ReaderController.zoom

            onSmallerRequested: ReaderSettings.smaller()
            onLargerRequested: ReaderSettings.larger()
            onThemePicked: function(id) {
                ReaderSettings.applyTheme(id)
            }
            onPageColorPicked: function(id) {
                ReaderSettings.pageColor = id
            }
            onFontFamilyPicked: function(family) {
                ReaderSettings.fontFamily = family
            }
            onBoldToggled: function(on) {
                ReaderSettings.bold = on
            }
            onJustifyToggled: function(on) {
                ReaderSettings.justify = on
            }
            onTwoPagesToggled: function(on) {
                ReaderSettings.twoPages = on
            }
            onLineHeightMoved: function(value) {
                ReaderSettings.lineHeight = value
            }
            onLetterSpacingMoved: function(value) {
                ReaderSettings.letterSpacing = value
            }
            onWordSpacingMoved: function(value) {
                ReaderSettings.wordSpacing = value
            }
            onMarginMoved: function(value) {
                ReaderSettings.margin = value
            }
            onResetRequested: ReaderSettings.reset()
            onFitPageRequested: ReaderController.fitPage()
            onFitWidthRequested: ReaderController.fitWidth()
            onZoomInRequested: ReaderController.zoomIn()
            onZoomOutRequested: ReaderController.zoomOut()
            onCloseRequested: settingsPopup.close()
        }
    }
}

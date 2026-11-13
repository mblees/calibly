// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Calibly.Theme
import Calibly.Controls

// The text settings of the reader ("Aa"), modelled on Apple Books: font
// size, themes, page color and under "Anpassen" font, bold, spacing,
// margins, justified text and the two-page spread. For a PDF it shows the
// zoom instead, as PDF pages are never reflowed. Presentation only: it shows
// the values it is given and reports what the user changes. Scrolls when it
// is higher than the space it gets.
//
// Properties:
//   pdf                 show the zoom of a PDF instead of the text settings
//   fontSizeStep, fontSizeSteps   0 … fontSizeSteps - 1
//   theme               id of the chosen theme, "" = changed by hand
//   themes              [{ id, fontFamily ("" = the book's fonts), bold, pageColor }]
//                       ids: original, ruhig, papier, kraeftig, sanft, fokus
//   pageColor           "weiss", "sepia", "grau" or "nacht" (Theme.readerPageColors)
//   fontFamily          "" = the book's own fonts ("Original")
//   fontFamilies        the fonts to choose from
//   bold, justify, twoPages
//   lineHeight (1.2 – 2.2), letterSpacing (0 – 0.12 em),
//   wordSpacing (0 – 0.5 em), margin (24 – 120 px)
//   zoomMode            PDF: 0 = whole page, 1 = page width, 2 = own zoom
//   zoom                PDF: 1 = 100 %
// Signals:
//   smallerRequested(), largerRequested()   font size
//   themePicked(string id), pageColorPicked(string id)
//   fontFamilyPicked(string family)         "" = Original
//   boldToggled(bool on), justifyToggled(bool on), twoPagesToggled(bool on)
//   lineHeightMoved(real value), letterSpacingMoved(real value),
//   wordSpacingMoved(real value), marginMoved(int value)
//   resetRequested()
//   fitPageRequested(), fitWidthRequested(), zoomInRequested(), zoomOutRequested()
//   closeRequested()                        the ✕ in the heading
Rectangle {
    id: root

    property bool pdf: false
    property int fontSizeStep: 2
    property int fontSizeSteps: 7
    property string theme
    property var themes: []
    property string pageColor: "nacht"
    property string fontFamily
    property list<string> fontFamilies: []
    property bool bold: false
    property real lineHeight: 1.6
    property real letterSpacing: 0
    property real wordSpacing: 0
    property int margin: 56
    property bool justify: true
    property bool twoPages: false
    property int zoomMode: 0
    property real zoom: 1

    signal smallerRequested()
    signal largerRequested()
    signal themePicked(string id)
    signal pageColorPicked(string id)
    signal fontFamilyPicked(string family)
    signal boldToggled(bool on)
    signal justifyToggled(bool on)
    signal twoPagesToggled(bool on)
    signal lineHeightMoved(real value)
    signal letterSpacingMoved(real value)
    signal wordSpacingMoved(real value)
    signal marginMoved(int value)
    signal resetRequested()
    signal fitPageRequested()
    signal fitWidthRequested()
    signal zoomInRequested()
    signal zoomOutRequested()
    signal closeRequested()

    readonly property var themeNames: ({
        "original": qsTr("Original"),
        "ruhig": qsTr("Ruhig"),
        "papier": qsTr("Papier"),
        "kraeftig": qsTr("Kräftig"),
        "sanft": qsTr("Sanft"),
        "fokus": qsTr("Fokus")
    })
    readonly property var pageColorNames: ({
        "weiss": qsTr("Weiß"),
        "sepia": qsTr("Sepia"),
        "grau": qsTr("Grau"),
        "nacht": qsTr("Nacht")
    })
    readonly property list<string> pageColorIds: ["weiss", "sepia", "grau", "nacht"]

    // Heading of a group, e.g. "THEMEN".
    component Label: Text {
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 1
    }

    // One line of "Anpassen": name on the left, the control on the right.
    component SettingRow: Item {
        id: settingRow

        property alias text: name.text
        default property alias control: holder.data

        width: parent ? parent.width : 0
        height: Theme.controlHeight

        Text {
            id: name

            width: Theme.textSettingsLabelWidth
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
        Item {
            id: holder

            anchors.left: name.right
            anchors.leftMargin: Theme.spacingS
            anchors.right: parent.right
            height: parent.height
        }
    }

    // A slider of "Anpassen" filling its row.
    component SettingSlider: AppSlider {
        width: parent ? parent.width : 0
        trackColor: Theme.toggleOff
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
    }

    // A switch of "Anpassen" at the right end of its row.
    component SettingSwitch: ToggleSwitch {
        anchors.right: parent ? parent.right : undefined
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
    }

    implicitWidth: Theme.textSettingsWidth
    implicitHeight: column.implicitHeight + 2 * Theme.spacingL
    radius: Theme.radiusL + Theme.spacingXxs
    color: Theme.surfaceRaised
    border.width: Theme.borderWidth
    border.color: Theme.inputBorder

    Flickable {
        id: flick

        anchors.fill: parent
        anchors.margins: Theme.borderWidth
        contentWidth: width
        contentHeight: column.implicitHeight + 2 * Theme.spacingL - 2 * Theme.borderWidth
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        Column {
            id: column

            x: Theme.spacingL
            y: Theme.spacingM
            width: flick.width - 2 * Theme.spacingL
            spacing: Theme.spacingM

            // --- heading ---------------------------------------------------------
            Item {
                width: parent.width
                height: Theme.controlHeight - Theme.spacingS

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.pdf ? qsTr("Ansicht") : qsTr("Texteinstellungen")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontLarge
                    font.weight: Font.Bold
                }
                IconButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.controlHeight - Theme.spacingS
                    height: width
                    iconName: "x"
                    text: qsTr("Schließen")
                    onClicked: root.closeRequested()
                }
            }

            // --- PDF: zoom -------------------------------------------------------
            Label {
                visible: root.pdf
                text: qsTr("Zoom")
            }
            Row {
                visible: root.pdf
                spacing: Theme.spacingS

                Chip {
                    text: qsTr("Ganze Seite")
                    checked: root.zoomMode === 0
                    onClicked: root.fitPageRequested()
                }
                Chip {
                    text: qsTr("Seitenbreite")
                    checked: root.zoomMode === 1
                    onClicked: root.fitWidthRequested()
                }
            }
            Row {
                visible: root.pdf
                spacing: Theme.spacingS

                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "minus"
                    text: qsTr("Verkleinern")
                    onClicked: root.zoomOutRequested()
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.fontStepButtonWidth
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("%1 %").arg(Math.round(root.zoom * 100))
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBody
                    font.features: { "tnum": 1 }
                }
                IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "plus"
                    text: qsTr("Vergrößern")
                    onClicked: root.zoomInRequested()
                }
            }

            // --- font size -------------------------------------------------------
            Item {
                visible: !root.pdf
                width: parent.width
                height: smaller.height

                T.AbstractButton {
                    id: smaller

                    width: Theme.fontStepButtonWidth
                    height: Theme.controlHeight + Theme.spacingXs
                    enabled: root.fontSizeStep > 0
                    text: qsTr("Schrift kleiner")
                    Accessible.name: smaller.text
                    onClicked: root.smallerRequested()

                    contentItem: Text {
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: "A"
                        opacity: smaller.enabled ? 1 : Theme.disabledOpacity
                        color: Theme.textPrimary
                        font.family: Theme.fontFamilySerif
                        font.pixelSize: Theme.fontBody
                    }
                    background: Rectangle {
                        radius: Theme.radiusM + Theme.spacingXxs
                        color: smaller.hovered ? Theme.surfaceSelected : Theme.surface
                        border.width: smaller.visualFocus ? Theme.focusWidth : Theme.borderWidth
                        border.color: smaller.visualFocus ? Theme.textPrimary : Theme.inputBorder
                    }
                }

                Row {
                    anchors.centerIn: parent
                    spacing: Theme.spacingS - 1

                    Accessible.role: Accessible.ProgressBar
                    Accessible.name: qsTr("Schriftgröße %1 von %2").arg(root.fontSizeStep + 1).arg(root.fontSizeSteps)

                    Repeater {
                        model: root.fontSizeSteps

                        delegate: Rectangle {
                            required property int index

                            width: Theme.fontStepDotSize
                            height: width
                            radius: width / 2
                            color: index <= root.fontSizeStep ? Theme.textPrimary : Theme.toggleOff
                        }
                    }
                }

                T.AbstractButton {
                    id: larger

                    anchors.right: parent.right
                    width: Theme.fontStepButtonWidth
                    height: smaller.height
                    enabled: root.fontSizeStep < root.fontSizeSteps - 1
                    text: qsTr("Schrift größer")
                    Accessible.name: larger.text
                    onClicked: root.largerRequested()

                    contentItem: Text {
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: "A"
                        opacity: larger.enabled ? 1 : Theme.disabledOpacity
                        color: Theme.textPrimary
                        font.family: Theme.fontFamilySerif
                        font.pixelSize: Theme.fontStepLargeSize
                    }
                    background: Rectangle {
                        radius: Theme.radiusM + Theme.spacingXxs
                        color: larger.hovered ? Theme.surfaceSelected : Theme.surface
                        border.width: larger.visualFocus ? Theme.focusWidth : Theme.borderWidth
                        border.color: larger.visualFocus ? Theme.textPrimary : Theme.inputBorder
                    }
                }
            }

            // --- themes ----------------------------------------------------------
            Label {
                visible: !root.pdf
                text: qsTr("Themen")
            }
            Grid {
                id: themeGrid

                visible: !root.pdf
                width: parent.width
                columns: 3
                spacing: Theme.spacingS

                Repeater {
                    model: root.themes

                    delegate: T.AbstractButton {
                        id: themeButton

                        required property var modelData

                        readonly property var colors: Theme.readerPageColors[themeButton.modelData.pageColor]
                                                      ?? Theme.readerPageColors["nacht"]
                        readonly property bool current: themeButton.modelData.id === root.theme

                        width: (themeGrid.width - 2 * themeGrid.spacing) / 3
                        height: Theme.themeSwatchHeight
                        text: root.themeNames[themeButton.modelData.id] ?? themeButton.modelData.id

                        Accessible.role: Accessible.RadioButton
                        Accessible.name: themeButton.text
                        Accessible.checked: themeButton.current

                        onClicked: root.themePicked(themeButton.modelData.id)

                        contentItem: Column {
                            topPadding: (themeButton.height - implicitHeight) / 2

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Aa"
                                color: themeButton.colors.text
                                font.family: themeButton.modelData.fontFamily !== ""
                                             ? themeButton.modelData.fontFamily : Theme.fontFamilySerif
                                font.bold: themeButton.modelData.bold
                                font.pixelSize: Theme.themeSwatchFontSize
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: themeButton.text
                                color: themeButton.colors.text
                                font.pixelSize: Theme.fontSmall + 1
                            }
                        }
                        background: Rectangle {
                            radius: Theme.radiusM + Theme.spacingXxs
                            color: themeButton.colors.page
                            border.width: Theme.borderWidth
                            border.color: Theme.inputBorder

                            // Ring around the chosen theme (and the focused one).
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -Theme.focusWidth - 1
                                radius: parent.radius + Theme.focusWidth + 1
                                visible: themeButton.current || themeButton.visualFocus
                                color: "transparent"
                                border.width: Theme.focusWidth
                                border.color: themeButton.current ? Theme.accent : Theme.textPrimary
                            }
                        }
                    }
                }
            }

            // --- page color ------------------------------------------------------
            Label {
                visible: !root.pdf
                text: qsTr("Seitenfarbe")
            }
            Row {
                id: colorRow

                visible: !root.pdf
                width: parent.width
                spacing: Theme.spacingS + Theme.spacingXxs

                Repeater {
                    model: root.pageColorIds

                    delegate: T.AbstractButton {
                        id: colorButton

                        required property string modelData

                        readonly property var colors: Theme.readerPageColors[colorButton.modelData]
                        readonly property bool current: colorButton.modelData === root.pageColor

                        width: (colorRow.width - 3 * colorRow.spacing) / 4
                        height: Theme.pageColorSwatchHeight
                        text: root.pageColorNames[colorButton.modelData]

                        Accessible.role: Accessible.RadioButton
                        Accessible.name: colorButton.text
                        Accessible.checked: colorButton.current

                        onClicked: root.pageColorPicked(colorButton.modelData)

                        contentItem: Text {
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: colorButton.text
                            color: colorButton.colors.text
                            font.pixelSize: Theme.fontSmall + 1
                        }
                        background: Rectangle {
                            radius: Theme.radiusM + Theme.spacingXxs
                            color: colorButton.colors.page
                            border.width: Theme.borderWidth
                            border.color: Theme.inputBorder

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -Theme.focusWidth - 1
                                radius: parent.radius + Theme.focusWidth + 1
                                visible: colorButton.current || colorButton.visualFocus
                                color: "transparent"
                                border.width: Theme.focusWidth
                                border.color: colorButton.current ? Theme.accent : Theme.textPrimary
                            }
                        }
                    }
                }
            }

            // --- "Anpassen" ------------------------------------------------------
            Rectangle {
                visible: !root.pdf
                width: parent.width
                height: Theme.borderWidth
                color: Theme.inputBorder
            }
            Label {
                visible: !root.pdf
                text: qsTr("Anpassen")
            }
            Column {
                visible: !root.pdf
                width: parent.width
                spacing: Theme.spacingS

                SettingRow {
                    text: qsTr("Schrift")

                    SelectMenu {
                        width: parent.width
                        anchors.verticalCenter: parent.verticalCenter
                        boxed: true
                        options: [qsTr("Original (Schrift des Buchs)")].concat(root.fontFamilies)
                        currentIndex: root.fontFamily === "" ? 0 : root.fontFamilies.indexOf(root.fontFamily) + 1
                        valueText: root.fontFamily === "" ? qsTr("Original") : ""
                        Accessible.name: qsTr("Schrift")
                        onActivated: function(index) {
                            root.fontFamilyPicked(index === 0 ? "" : root.fontFamilies[index - 1])
                        }
                    }
                }
                SettingRow {
                    text: qsTr("Fett")

                    SettingSwitch {
                        text: qsTr("Fett")
                        checked: root.bold
                        onToggled: root.boldToggled(checked)
                    }
                }
                SettingRow {
                    text: qsTr("Zeilenabstand")

                    SettingSlider {
                        from: 1.2
                        to: 2.2
                        stepSize: 0.1
                        value: root.lineHeight
                        Accessible.name: qsTr("Zeilenabstand")
                        onMoved: root.lineHeightMoved(value)
                    }
                }
                SettingRow {
                    text: qsTr("Zeichenabstand")

                    SettingSlider {
                        from: 0
                        to: 0.12
                        stepSize: 0.02
                        value: root.letterSpacing
                        Accessible.name: qsTr("Zeichenabstand")
                        onMoved: root.letterSpacingMoved(value)
                    }
                }
                SettingRow {
                    text: qsTr("Wortabstand")

                    SettingSlider {
                        from: 0
                        to: 0.5
                        stepSize: 0.1
                        value: root.wordSpacing
                        Accessible.name: qsTr("Wortabstand")
                        onMoved: root.wordSpacingMoved(value)
                    }
                }
                SettingRow {
                    text: qsTr("Ränder")

                    SettingSlider {
                        from: 24
                        to: 120
                        stepSize: 8
                        value: root.margin
                        Accessible.name: qsTr("Ränder")
                        onMoved: root.marginMoved(Math.round(value))
                    }
                }
                SettingRow {
                    text: qsTr("Blocksatz")

                    SettingSwitch {
                        text: qsTr("Blocksatz")
                        checked: root.justify
                        onToggled: root.justifyToggled(checked)
                    }
                }
                SettingRow {
                    text: qsTr("Doppelseite")

                    Badge {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Im Vollbild")
                        color: Theme.textSecondary
                        backgroundColor: Theme.surface
                    }
                    SettingSwitch {
                        text: qsTr("Doppelseite im Vollbild")
                        checked: root.twoPages
                        onToggled: root.twoPagesToggled(checked)
                    }
                }
                LinkButton {
                    text: qsTr("Zurücksetzen")
                    font.pixelSize: Theme.fontBody
                    onClicked: root.resetRequested()
                }
            }
        }
    }
}

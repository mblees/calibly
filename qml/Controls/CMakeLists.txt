# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

qt_add_library(calibly_controls STATIC)

qt_add_qml_module(calibly_controls
    URI Calibly.Controls
    VERSION 1.0
    QML_FILES
        AppButton.qml
        IconButton.qml
        ProgressLine.qml
        Badge.qml
        Chip.qml
        ChipGroup.qml
        SearchField.qml
        SelectMenu.qml
        AppSlider.qml
        SideDrawer.qml
        Tabs.qml
        RatingStars.qml
        FormField.qml
        InputField.qml
        TextAreaField.qml
        TagField.qml
        SeekBar.qml
        Spinner.qml
        LinkButton.qml
        ConfirmDialog.qml
        AppMenu.qml
        AppMenuItem.qml
        AppMenuSeparator.qml
        ToggleSwitch.qml
        ColorWheel.qml
        ColorSwatch.qml
        TimeFormat.js
)

target_link_libraries(calibly_controls PRIVATE Qt6::Quick calibly_theme)

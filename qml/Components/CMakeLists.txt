# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

qt_add_library(calibly_components STATIC)

qt_add_qml_module(calibly_components
    URI Calibly.Components
    VERSION 1.0
    QML_FILES
        SidebarNav.qml
        CoverArt.qml
        CoverPicker.qml
        CoverTile.qml
        FormatCard.qml
        MetadataForm.qml
        PlayerControls.qml
        PlaybackOptions.qml
        ChapterList.qml
        MiniPlayer.qml
        DropHint.qml
        ImportQueue.qml
        ContinueCard.qml
        ContinueMenu.qml
        BulkActionBar.qml
        WorkMenu.qml
        SearchPanel.qml
        TableOfContents.qml
        TextSettings.qml
        ReaderBar.qml
        LibraryProblem.qml
        UpdateHint.qml
        ExportHint.qml
        SeriesDialog.qml
)

# calibly_models: only for the enum WorkState (the state of a cover).
target_link_libraries(calibly_components PRIVATE Qt6::Quick calibly_theme calibly_controls calibly_models)

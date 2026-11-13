# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

qt_add_library(calibly_views STATIC)

qt_add_qml_module(calibly_views
    URI Calibly.Views
    VERSION 1.0
    QML_FILES
        ViewBase.qml
        PlaceholderView.qml
        LibraryView.qml
        DetailDrawer.qml
        MetadataEditor.qml
        SeriesOrderDialog.qml
        PlayerView.qml
        PlayerBar.qml
        ContinueRow.qml
        ContinueView.qml
        SeriesView.qml
        AuthorsView.qml
        NewView.qml
        ReaderView.qml
        SettingsView.qml
        ComponentGallery.qml
)

target_link_libraries(calibly_views PRIVATE
    Qt6::Quick
    calibly_controllers
    calibly_models
    calibly_theme
    calibly_controls
    calibly_components
)

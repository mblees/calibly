qt_add_library(calibly_components STATIC)

qt_add_qml_module(calibly_components
    URI Calibly.Components
    VERSION 1.0
    QML_FILES
        SidebarNav.qml
        CoverArt.qml
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
        BulkActionBar.qml
        WorkMenu.qml
)

# calibly_models: only for the enum WorkState (the state of a cover).
target_link_libraries(calibly_components PRIVATE Qt6::Quick calibly_theme calibly_controls calibly_models)

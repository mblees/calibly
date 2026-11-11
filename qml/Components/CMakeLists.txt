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
)

target_link_libraries(calibly_components PRIVATE Qt6::Quick calibly_theme calibly_controls)

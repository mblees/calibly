qt_add_library(calibly_theme STATIC)

set_source_files_properties(Theme.qml Icons.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(calibly_theme
    URI Calibly.Theme
    VERSION 1.0
    QML_FILES
        Theme.qml
        Icons.qml
        Icon.qml
)

target_link_libraries(calibly_theme PRIVATE Qt6::Quick)

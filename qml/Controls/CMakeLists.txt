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
)

target_link_libraries(calibly_controls PRIVATE Qt6::Quick calibly_theme)

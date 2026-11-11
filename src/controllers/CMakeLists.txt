# C++ controllers that QML views can use (layer below the QML modules).
qt_add_library(calibly_controllers STATIC)

qt_add_qml_module(calibly_controllers
    URI Calibly.Controllers
    VERSION 1.0
    SOURCES
        languagemanager.h
        languagemanager.cpp
)

target_include_directories(calibly_controllers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calibly_controllers PUBLIC Qt6::Qml)

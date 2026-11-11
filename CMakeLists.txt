cmake_minimum_required(VERSION 3.25)

project(calibly
    VERSION 0.1.0
    DESCRIPTION "One library for ebooks and audiobooks"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Phase 0: Qt Quick is the only dependency (see docs/UI-Development-Plan.md).
# LinguistTools are Qt's own build tools for translations (lupdate, lrelease).
# Test (Qt Test) is only used by the unit tests in tests/.
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 LinguistTools Test)

# Texts in the source code are German. Every further UI language is listed
# in I18N_TRANSLATED_LANGUAGES and gets its own i18n/calibly_<code>.ts file.
qt_standard_project_setup(REQUIRES 6.8
    I18N_SOURCE_LANGUAGE de
    I18N_TRANSLATED_LANGUAGES en
)

# Phase 4: SQLite for the library database (Qt SQL), Qt PDF for covers of PDF
# files (optional: only used when the Qt PDF module is installed).
find_package(Qt6 6.8 REQUIRED COMPONENTS Sql CorePrivate)
find_package(Qt6 6.8 QUIET OPTIONAL_COMPONENTS Pdf)
if(TARGET Qt6::Pdf)
    message(STATUS "calibly: Qt PDF found, PDF covers are enabled")
else()
    message(STATUS "calibly: Qt PDF not found, PDFs get placeholder covers")
endif()

# Phase 6: Qt Multimedia plays the audiobooks (Qt Maintenance Tool:
# Qt 6.x -> Additional Libraries -> Qt Multimedia).
find_package(Qt6 6.8 REQUIRED COMPONENTS Multimedia)

include(cmake/Dependencies.cmake)

# All QML modules are written below <build>/qml, so that tools like qmllint
# find every module (Calibly, Calibly.Theme, Calibly.Controls, ...).
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

# C++ layers, bottom to top: core (data classes) -> backend (data sources)
# and import (reading and copying files) -> models (Qt item models)
# -> controllers (used by the QML views).
add_subdirectory(src/core)
add_subdirectory(src/backend)
add_subdirectory(src/import)
add_subdirectory(src/models)
add_subdirectory(src/controllers)
add_subdirectory(qml)

enable_testing()
add_subdirectory(tests)

qt_add_executable(calibly
    src/main.cpp
)

qt_add_qml_module(calibly
    URI Calibly
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

# Sample library for development (MockBackend), see resources/mock.
file(GLOB mock_covers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS resources/mock/covers/*.jpg)
qt_add_resources(calibly "mock_data"
    PREFIX /mock
    BASE resources/mock
    FILES
        resources/mock/mock.json
        ${mock_covers}
)

set_target_properties(calibly PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(calibly PRIVATE
    CALIBLY_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(calibly PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    calibly_controllers
    calibly_controllersplugin
    calibly_modelsplugin
    calibly_themeplugin
    calibly_controlsplugin
    calibly_componentsplugin
    calibly_viewsplugin
)

# Translations (Qt Linguist):
#   update_translations   extracts all qsTr() texts into i18n/*.ts (lupdate)
#   release_translations  compiles them to .qm files (lrelease, runs on every build)
# The .qm files are embedded into the app under :/i18n. Qt's own texts
# (e.g. standard dialogs) are merged in, so no extra files need to be shipped.
qt_add_translations(calibly
    SOURCE_TARGETS
        calibly
        calibly_controllers
        calibly_theme
        calibly_controls
        calibly_components
        calibly_views
    TS_FILE_DIR i18n
    TS_FILE_BASE calibly
    # Drop texts that no longer exist in the code from the .ts files.
    LUPDATE_OPTIONS -no-obsolete
    MERGE_QT_TRANSLATIONS
    QT_TRANSLATION_CATALOGS qtbase qtdeclarative
)

# Convenience targets for development:
#   cmake --build --preset mingw-debug --target run
#   cmake --build --preset mingw-debug --target run-gallery
add_custom_target(run
    COMMAND calibly
    USES_TERMINAL
)
add_custom_target(run-gallery
    COMMAND calibly --gallery
    USES_TERMINAL
)

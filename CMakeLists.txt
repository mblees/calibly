cmake_minimum_required(VERSION 3.28)

project(calibly
    VERSION 0.2.0
    DESCRIPTION "One library for ebooks and audiobooks"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release information (see docs/Release-Plan.md). The version shown to users
# is PROJECT_VERSION plus the suffix, e.g. "0.1.0-beta"; an empty suffix
# makes a final release.
set(CALIBLY_VERSION_SUFFIX "beta")
set(CALIBLY_PUBLISHER "mblees")
set(CALIBLY_COPYRIGHT "Copyright (C) 2026 mblees")
# The releases are published in the working repository (decided 2026-11-12).
set(CALIBLY_RELEASES_REPO "mblees/calibly")
set(CALIBLY_RELEASES_URL "https://github.com/${CALIBLY_RELEASES_REPO}/releases")
if(CALIBLY_VERSION_SUFFIX)
    set(CALIBLY_DISPLAY_VERSION "${PROJECT_VERSION}-${CALIBLY_VERSION_SUFFIX}")
else()
    set(CALIBLY_DISPLAY_VERSION "${PROJECT_VERSION}")
endif()

# Development tools: the options --mock, --gallery, --screenshot and --size
# and the built-in sample library. Off in the release preset, so they are
# not shipped.
option(CALIBLY_DEV_TOOLS "Build the development options and the sample library into calibly.exe" ON)

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

# Release step 3: Qt Network lets a second start of calibly bring the running
# window to the front (LibraryLock).
find_package(Qt6 6.8 REQUIRED COMPONENTS Network)

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
add_subdirectory(src/reader)
add_subdirectory(src/models)
add_subdirectory(src/controllers)
add_subdirectory(qml)

enable_testing()
add_subdirectory(tests)

qt_add_executable(calibly
    src/main.cpp
)

# Icon and version information of calibly.exe (resources/windows).
if(WIN32)
    enable_language(RC)
    set(CALIBLY_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/calibly.ico")
    configure_file(resources/windows/calibly.rc.in ${CMAKE_CURRENT_BINARY_DIR}/calibly.rc @ONLY)
    target_sources(calibly PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/calibly.rc)
    set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/calibly.rc APPEND PROPERTY OBJECT_DEPENDS ${CALIBLY_ICON_FILE})
endif()

qt_add_qml_module(calibly
    URI Calibly
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

# Sample library for development (MockBackend), see resources/mock.
if(CALIBLY_DEV_TOOLS)
    file(GLOB mock_covers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS resources/mock/covers/*.jpg)
    qt_add_resources(calibly "mock_data"
        PREFIX /mock
        BASE resources/mock
        FILES
            resources/mock/mock.json
            ${mock_covers}
    )
    target_compile_definitions(calibly PRIVATE CALIBLY_DEV_TOOLS)
endif()

set_target_properties(calibly PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(calibly PRIVATE
    CALIBLY_VERSION="${CALIBLY_DISPLAY_VERSION}"
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
if(CALIBLY_DEV_TOOLS)
    add_custom_target(run-gallery
        COMMAND calibly --gallery
        USES_TERMINAL
    )
endif()

# --- Release: installed folder and installer ---------------------------------------
#   cmake --install build/mingw-release --prefix build/mingw-release/deploy
# writes calibly.exe with everything it needs (Qt and MinGW DLLs, QML modules,
# plugins) into one folder; installer/calibly.iss packs that folder into the
# setup file. tools/make-release.ps1 runs all steps.
# Everything lies next to calibly.exe (DLLs, plugins/, qml/), as Windows
# programs usually do; Qt finds its plugins and QML modules there by itself.
install(TARGETS calibly
    RUNTIME DESTINATION .
)
install(FILES
    resources/icon/calibly.ico
    LICENSE.txt
    LICENSE.de.txt
    DESTINATION .
)
install(DIRECTORY licenses/texts/
    DESTINATION licenses
)

# Qt's deployment (windeployqt underneath). Translations of Qt are already
# built into calibly; Mesa's software OpenGL is not needed (Qt Quick uses
# Direct3D 11, which Windows can also run without a graphics card).
qt_generate_deploy_qml_app_script(
    TARGET calibly
    OUTPUT_SCRIPT calibly_deploy_script
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-opengl-sw --no-system-d3d-compiler --compiler-runtime --skip-plugin-types qmltooling
)
# The deployment puts the DLLs into bin/ unless told otherwise.
install(CODE [[set(QT_DEPLOY_BIN_DIR .)]])
install(SCRIPT ${calibly_deploy_script})

# Qt Quick Dialogs pulls in every Qt Quick Controls style; calibly always uses
# "Basic" (main.cpp), so the others are left out (about 20 MB).
install(CODE [[
    foreach(style Fusion Imagine Material Universal FluentWinUI3 Windows)
        file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/qml/QtQuick/Controls/${style}")
        file(GLOB style_libraries "${CMAKE_INSTALL_PREFIX}/Qt6QuickControls2${style}*.dll")
        if(style_libraries)
            file(REMOVE ${style_libraries})
        endif()
    endforeach()
    # calibly only uses SQLite.
    foreach(driver ibase mimer oci odbc psql)
        file(REMOVE "${CMAKE_INSTALL_PREFIX}/plugins/sqldrivers/qsql${driver}.dll")
    endforeach()
]])

# THIRD-PARTY-NOTICES.txt for exactly the files in the folder, from Qt's
# software bill of materials and documentation (see the script). Needs
# Python 3 and the Qt documentation (Qt Maintenance Tool: Qt -> Qt 6.x ->
# Documentation is installed by default).
find_package(Python3 COMPONENTS Interpreter)
get_filename_component(qt_root "${QT6_INSTALL_PREFIX}/../.." ABSOLUTE)
set(CALIBLY_QT_DOCS_DIR "${qt_root}/Docs/Qt-${Qt6_VERSION}" CACHE PATH "Qt documentation, for the third-party notices")
if(Python3_FOUND)
    install(CODE "
        execute_process(
            COMMAND \"${Python3_EXECUTABLE}\" -I \"${CMAKE_CURRENT_SOURCE_DIR}/tools/generate-third-party-notices.py\"
                    \"\${CMAKE_INSTALL_PREFIX}\" \"${QT6_INSTALL_PREFIX}\" \"${CALIBLY_QT_DOCS_DIR}\" \"${CALIBLY_DISPLAY_VERSION}\"
            COMMAND_ERROR_IS_FATAL ANY
        )
    ")
else()
    install(CODE [[message(WARNING "Python 3 was not found: THIRD-PARTY-NOTICES.txt is missing in the installed folder.")]])
endif()

configure_file(installer/version.iss.in ${CMAKE_CURRENT_BINARY_DIR}/installer/version.iss @ONLY)

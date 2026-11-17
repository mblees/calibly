# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

# C++ controllers that QML views can use (layer below the QML modules).
qt_add_library(calibly_controllers STATIC)

qt_add_qml_module(calibly_controllers
    URI Calibly.Controllers
    VERSION 1.0
    DEPENDENCIES
        Calibly.Models
    SOURCES
        languagemanager.h
        languagemanager.cpp
        librarycontroller.h
        librarycontroller.cpp
        librarylock.h
        librarylock.cpp
        updatechecker.h
        updatechecker.cpp
        appinfo.h
        appinfo.cpp
        importcontroller.h
        importcontroller.cpp
        workeditor.h
        workeditor.cpp
        playercontroller.h
        playercontroller.cpp
        readersettings.h
        readersettings.cpp
        readercontroller.h
        readercontroller.cpp
        bookpage.h
        bookpage.cpp
        appearancesettings.h
        appearancesettings.cpp
        exportcontroller.h
        exportcontroller.cpp
        mediacontrols.h
        mediacontrols.cpp
)

target_include_directories(calibly_controllers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt GUI for QDesktopServices (open files with the default application),
# Qt Multimedia for the audiobook player, Qt Network for LibraryLock.
target_link_libraries(calibly_controllers PUBLIC Qt6::Qml Qt6::Gui Qt6::Quick Qt6::Multimedia Qt6::Network calibly_models
    calibly_backend calibly_import calibly_reader calibly_core)

# The media keys and the media overlay of Windows (MediaControls): WinRT
# activation, streams over memory.
if(WIN32)
    target_link_libraries(calibly_controllers PRIVATE runtimeobject shcore shlwapi)
endif()

# The releases page, the source code and the copyright line (see the top of the
# root CMakeLists.txt), for the update check and "Über calibly".
target_compile_definitions(calibly_controllers PRIVATE
    CALIBLY_RELEASES_REPO="${CALIBLY_RELEASES_REPO}"
    CALIBLY_RELEASES_URL="${CALIBLY_RELEASES_URL}"
    CALIBLY_SOURCE_URL="${CALIBLY_SOURCE_URL}"
    CALIBLY_COPYRIGHT="${CALIBLY_COPYRIGHT}"
)

# Qt PDF draws PDFs in the reader; without it PDFs open in the default application.
if(TARGET Qt6::Pdf)
    target_link_libraries(calibly_controllers PUBLIC Qt6::Pdf)
    target_compile_definitions(calibly_controllers PRIVATE CALIBLY_HAS_QTPDF)
endif()

# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

# Data sources behind the LibraryBackend interface. No UI, no QML.
qt_add_library(calibly_backend STATIC
    librarybackend.h
    mockbackend.h
    mockbackend.cpp
    sqlitebackend.h
    sqlitebackend.cpp
)

target_include_directories(calibly_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Qt GUI for QImageReader (cover sizes), Qt SQL for the library database.
target_link_libraries(calibly_backend PUBLIC calibly_core PRIVATE Qt6::Gui Qt6::Sql)

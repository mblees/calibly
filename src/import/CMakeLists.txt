# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

# Import of files into the library: metadata readers (EPUB, audio, PDF,
# file names), grouping into works, copying into the library folder.
# No UI, no QML.
qt_add_library(calibly_import STATIC
    filemetadata.h
    filenameparser.h
    filenameparser.cpp
    epubreader.h
    epubreader.cpp
    audioreader.h
    audioreader.cpp
    pdfreader.h
    pdfreader.cpp
    metadatareader.h
    metadatareader.cpp
    importplanner.h
    importplanner.cpp
    libraryfolder.h
    libraryfolder.cpp
    importworker.h
    importworker.cpp
)

target_include_directories(calibly_import PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calibly_import
    PUBLIC calibly_core Qt6::Gui
    # QZipReader for EPUB (Qt-internal API, see epubreader.cpp)
    PRIVATE Qt6::CorePrivate calibly_taglib
)

if(TARGET Qt6::Pdf)
    target_link_libraries(calibly_import PRIVATE Qt6::Pdf)
    target_compile_definitions(calibly_import PRIVATE CALIBLY_HAS_QTPDF)
endif()

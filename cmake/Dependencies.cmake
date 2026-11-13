# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

# Third-party libraries that are not part of Qt. They are downloaded and built
# together with calibly, so nothing has to be installed by hand.

include(FetchContent)

# --- TagLib: metadata, cover and length of audio files (M4B, MP3, ...) -------
# License: LGPL 2.1 / MPL 1.1; calibly (GPL 3) uses it under the LGPL 2.1.
# Built as a static library.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(taglib
    GIT_REPOSITORY https://github.com/taglib/taglib.git
    GIT_TAG v2.3.2
    GIT_SHALLOW TRUE
    GIT_SUBMODULES 3rdparty/utfcpp
    SYSTEM
    # Linked into calibly.exe: its headers and library are not installed.
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(taglib)

# TagLib only exports its include directories for installed builds. For the
# build-tree target used here, calibly_taglib adds the header folders we use.
add_library(calibly_taglib INTERFACE)
target_link_libraries(calibly_taglib INTERFACE tag)
target_include_directories(calibly_taglib SYSTEM INTERFACE
    ${taglib_BINARY_DIR}
    ${taglib_SOURCE_DIR}/taglib
    ${taglib_SOURCE_DIR}/taglib/toolkit
    ${taglib_SOURCE_DIR}/taglib/mp4
    ${taglib_SOURCE_DIR}/taglib/mpeg
    ${taglib_SOURCE_DIR}/taglib/mpeg/id3v2
    ${taglib_SOURCE_DIR}/taglib/mpeg/id3v2/frames
)

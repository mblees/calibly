# This file is part of calibly.
# Copyright (C) 2026 mblees
# SPDX-License-Identifier: GPL-3.0-only

# Qt item models for the UI (QML module Calibly.Models).
qt_add_library(calibly_models STATIC)

qt_add_qml_module(calibly_models
    URI Calibly.Models
    VERSION 1.0
    SOURCES
        workmodel.h
        workmodel.cpp
        workfiltermodel.h
        workfiltermodel.cpp
        workdetails.h
        workdetails.cpp
        importqueuemodel.h
        importqueuemodel.cpp
        workstate.h
        continuemodel.h
        continuemodel.cpp
)

target_include_directories(calibly_models PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calibly_models PUBLIC calibly_core Qt6::Qml)

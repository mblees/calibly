# Plain data classes and helpers (Work, Format, text helpers). No UI, no QML.
qt_add_library(calibly_core STATIC
    work.h
    work.cpp
    textutils.h
    textutils.cpp
    audiotimeline.h
    audiotimeline.cpp
    workmerge.h
    workmerge.cpp
    readingposition.h
    readingposition.cpp
)

target_include_directories(calibly_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calibly_core PUBLIC Qt6::Core)

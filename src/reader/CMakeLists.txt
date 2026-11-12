# The ebook reader without UI (phase 10): EPUB and text books, the HTML
# preparation and the page layout with QTextDocument (option A).
qt_add_library(calibly_reader STATIC
    booksource.h
    booksource.cpp
    epubbook.h
    epubbook.cpp
    textbook.h
    textbook.cpp
    htmlprep.h
    htmlprep.cpp
    booklayout.h
    booklayout.cpp
)

target_include_directories(calibly_reader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calibly_reader
    PUBLIC calibly_core Qt6::Gui
    # QZipReader for EPUB (Qt-internal API, as in the import)
    PRIVATE Qt6::CorePrivate
)

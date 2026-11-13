# Unit tests (Qt Test). Run all with:
#   ctest --preset mingw-debug
qt_add_executable(tst_workfiltermodel tst_workfiltermodel.cpp)
target_link_libraries(tst_workfiltermodel PRIVATE Qt6::Test calibly_models)
add_test(NAME tst_workfiltermodel COMMAND tst_workfiltermodel)

qt_add_executable(tst_workdetails tst_workdetails.cpp)
target_link_libraries(tst_workdetails PRIVATE Qt6::Test calibly_models calibly_controllers)
add_test(NAME tst_workdetails COMMAND tst_workdetails)

qt_add_executable(tst_workeditor tst_workeditor.cpp)
target_link_libraries(tst_workeditor PRIVATE Qt6::Test calibly_models calibly_controllers)
add_test(NAME tst_workeditor COMMAND tst_workeditor)

qt_add_executable(tst_player tst_player.cpp)
target_link_libraries(tst_player PRIVATE Qt6::Test calibly_models calibly_controllers)
add_test(NAME tst_player COMMAND tst_player)

qt_add_executable(tst_audiotimeline tst_audiotimeline.cpp)
target_link_libraries(tst_audiotimeline PRIVATE Qt6::Test calibly_core)
add_test(NAME tst_audiotimeline COMMAND tst_audiotimeline)

qt_add_executable(tst_import tst_import.cpp)
# QZipWriter (Qt-internal) creates a small EPUB for the test.
# TagLib writes the tags and chapters of the test MP3 files.
target_link_libraries(tst_import PRIVATE Qt6::Test Qt6::CorePrivate Qt6::Sql calibly_import calibly_backend
    calibly_taglib)
add_test(NAME tst_import COMMAND tst_import)
# The PDF test only runs when calibly is built with Qt PDF.
target_link_libraries(tst_import PRIVATE Qt6::Gui)

qt_add_executable(tst_importcontroller tst_importcontroller.cpp)
target_link_libraries(tst_importcontroller PRIVATE Qt6::Test calibly_models calibly_controllers)
add_test(NAME tst_importcontroller COMMAND tst_importcontroller)

qt_add_executable(tst_continuemodel tst_continuemodel.cpp)
target_link_libraries(tst_continuemodel PRIVATE Qt6::Test calibly_models)
add_test(NAME tst_continuemodel COMMAND tst_continuemodel)

qt_add_executable(tst_workmerge tst_workmerge.cpp)
target_link_libraries(tst_workmerge PRIVATE Qt6::Test calibly_core)
add_test(NAME tst_workmerge COMMAND tst_workmerge)

qt_add_executable(tst_librarycontroller tst_librarycontroller.cpp)
target_link_libraries(tst_librarycontroller PRIVATE Qt6::Test calibly_models calibly_controllers)
add_test(NAME tst_librarycontroller COMMAND tst_librarycontroller)

qt_add_executable(tst_reader tst_reader.cpp)
# QZipWriter (Qt-internal) writes the test EPUBs.
target_link_libraries(tst_reader PRIVATE Qt6::Test Qt6::CorePrivate calibly_reader)
add_test(NAME tst_reader COMMAND tst_reader)

qt_add_executable(tst_readercontroller tst_readercontroller.cpp)
target_link_libraries(tst_readercontroller PRIVATE Qt6::Test Qt6::CorePrivate calibly_models calibly_controllers)
add_test(NAME tst_readercontroller COMMAND tst_readercontroller)

qt_add_executable(tst_logfile tst_logfile.cpp)
target_link_libraries(tst_logfile PRIVATE Qt6::Test calibly_core)
add_test(NAME tst_logfile COMMAND tst_logfile)

qt_add_executable(tst_export tst_export.cpp)
target_link_libraries(tst_export PRIVATE Qt6::Test calibly_models calibly_controllers)
add_test(NAME tst_export COMMAND tst_export)

qt_add_executable(tst_appsettings tst_appsettings.cpp)
target_link_libraries(tst_appsettings PRIVATE Qt6::Test calibly_controllers)
add_test(NAME tst_appsettings COMMAND tst_appsettings)

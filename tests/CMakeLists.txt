# Unit tests (Qt Test). Run all with:
#   ctest --preset mingw-debug
qt_add_executable(tst_workfiltermodel tst_workfiltermodel.cpp)
target_link_libraries(tst_workfiltermodel PRIVATE Qt6::Test calibly_models)
add_test(NAME tst_workfiltermodel COMMAND tst_workfiltermodel)

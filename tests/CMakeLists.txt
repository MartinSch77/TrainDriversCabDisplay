add_executable(tst_core tst_core.cpp rd_test.h)
target_link_libraries(tst_core PRIVATE traincore::traincore)
add_test(NAME tst_core COMMAND tst_core)

add_executable(tst_benchmarks tst_benchmarks.cpp rd_test.h)
target_link_libraries(tst_benchmarks PRIVATE traincore::traincore)
add_test(NAME tst_benchmarks COMMAND tst_benchmarks)

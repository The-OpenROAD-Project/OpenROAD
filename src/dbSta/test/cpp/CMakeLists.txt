include(openroad)

add_executable(TestHconn TestHconn.cpp)
target_link_libraries(TestHconn
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        tst
        utl_lib
        ${TCL_LIBRARY}
)

target_include_directories(TestHconn
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestHconn
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestHconn
)


add_executable(TestDbSta TestDbSta.cc)
target_link_libraries(TestDbSta
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        odb
        tst_integrated_fixture
        ${TCL_LIBRARY}
)

target_include_directories(TestDbSta
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestDbSta
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestDbSta
)

add_executable(TestReadVerilog TestReadVerilog.cpp)
target_link_libraries(TestReadVerilog
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        odb
        tst_integrated_fixture
        ${TCL_LIBRARY}
)

target_include_directories(TestReadVerilog
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestReadVerilog
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestReadVerilog
)

add_executable(TestWriteVerilog TestWriteVerilog.cpp)
target_link_libraries(TestWriteVerilog
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        odb
        tst_integrated_fixture
        ${TCL_LIBRARY}
)

target_include_directories(TestWriteVerilog
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestWriteVerilog
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestWriteVerilog
)

# Hierarchy conformance: both link modes LEC'd against the input netlist.
# Needs the external kepler-formal binary at run time (resolved from
# $KEPLER_FORMAL or $PATH); without it the tests fail with an actionable
# message rather than skipping.
add_executable(TestHierConformance TestHierConformance.cpp)
target_link_libraries(TestHierConformance
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        odb
        tst_base
        tst_lec
        tst_loaded_design
        ${TCL_LIBRARY}
)

gtest_discover_tests(TestHierConformance
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestHierConformance
)

# Structural conformance: the emitted netlist is diffed against the input
# netlist for the fidelity aspects a LEC cannot see (names, port order, module
# set, declared objects, instance bindings). Needs no external tool.
add_executable(TestHierStructural TestHierStructural.cpp)
target_link_libraries(TestHierStructural
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        odb
        tst_base
        tst_loaded_design
        ${TCL_LIBRARY}
)

gtest_discover_tests(TestHierStructural
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestHierStructural
)

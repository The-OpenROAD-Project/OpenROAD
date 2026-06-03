include(openroad)

or_integration_tests(
  "est"
  TESTS
    make_parasitics1
    make_parasitics2
    make_parasitics3
    make_parasitics4
    make_parasitics5
    make_parasitics6
    make_parasitics7
)

add_executable(TestEstimateParasitics cpp/TestEstimateParasitics.cc)
target_link_libraries(TestEstimateParasitics
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        est_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        odb
        tst
        ${TCL_LIBRARY}
)

gtest_discover_tests(TestEstimateParasitics
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestEstimateParasitics
)

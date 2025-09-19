include(openroad)

add_executable(TestBufRem1 TestBufferRemoval.cc)
target_link_libraries(TestBufRem1
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
        ${TCL_LIBRARY}
)

target_include_directories(TestBufRem1
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/rsz/src
)

gtest_discover_tests(TestBufRem1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestBufRem1
)


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
        tst
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

add_executable(TestBufferRemoval2 TestBufferRemoval2.cc)
target_link_libraries(TestBufferRemoval2
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
        tst
        ${TCL_LIBRARY}
)

target_include_directories(TestBufferRemoval2
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/rsz/src
)

gtest_discover_tests(TestBufferRemoval2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestBufferRemoval2
)


add_executable(TestBufferRemoval3 TestBufferRemoval3.cc)
target_link_libraries(TestBufferRemoval3
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
        tst
        ${TCL_LIBRARY}
)

target_include_directories(TestBufferRemoval3
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/rsz/src
)

gtest_discover_tests(TestBufferRemoval3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestBufferRemoval3
)

add_executable(TestInsertBuffer TestInsertBuffer.cpp)
target_link_libraries(TestInsertBuffer
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        rsz_lib
        odb
        odb_test_helper
        tst
        ${TCL_LIBRARY}
)

target_include_directories(TestInsertBuffer
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/rsz/src
)

gtest_discover_tests(TestInsertBuffer
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestInsertBuffer
)

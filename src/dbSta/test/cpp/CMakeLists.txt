include(openroad)

add_executable(TestHconn TestHconn.cpp)
target_link_libraries(TestHconn
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
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

add_executable(TestRename TestRename.cpp)
target_link_libraries(TestRename
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        ${TCL_LIBRARY}
)

target_include_directories(TestRename
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
      ${PROJECT_SOURCE_DIR}/src/odb/src/db
)

gtest_discover_tests(TestRename
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestRename
)


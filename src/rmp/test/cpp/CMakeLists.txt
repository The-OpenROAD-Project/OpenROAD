include(openroad)

add_executable(RmpGTests TestAbc.cc)
target_link_libraries(RmpGTests 
        OpenSTA
        gtest
        gmock
        gtest_main
        libabc
        rmp_abc_library
        utl_lib
)

gtest_discover_tests(RmpGTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test RmpGTests
)

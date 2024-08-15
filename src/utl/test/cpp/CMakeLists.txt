include("openroad")

set(TEST_LIBS
  GTest::gtest
  GTest::gtest_main
  utl_lib
)

add_executable(TestCFileUtils TestCFileUtils.cpp)

target_link_libraries(TestCFileUtils ${TEST_LIBS})

gtest_discover_tests(TestCFileUtils
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test
  TestCFileUtils
)

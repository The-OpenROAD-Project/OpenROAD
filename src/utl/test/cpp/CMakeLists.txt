include("openroad")

set(TEST_LIBS
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
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


add_executable(TestMetrics TestMetrics.cpp)

target_link_libraries(TestMetrics ${TEST_LIBS})

gtest_discover_tests(TestMetrics
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test
  TestMetrics
)

add_executable(TestAlgorithms TestAlgorithms.cpp)

target_link_libraries(TestAlgorithms ${TEST_LIBS})

gtest_discover_tests(TestAlgorithms
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test
  TestAlgorithms
)

add_executable(TestSuppressStdout TestSuppressStdout.cpp)

target_link_libraries(TestSuppressStdout ${TEST_LIBS})

gtest_discover_tests(TestSuppressStdout
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test
  TestSuppressStdout
)

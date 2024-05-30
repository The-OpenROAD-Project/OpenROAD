include("openroad")

set(TEST_LIBS
  gtest
  gtest_main
  utl
)

add_executable(CFileUtilsTest CFileUtilsTest.cpp)

target_link_libraries(CFileUtilsTest ${TEST_LIBS})

gtest_discover_tests(CFileUtilsTest
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test
  CFileUtilsTest
)

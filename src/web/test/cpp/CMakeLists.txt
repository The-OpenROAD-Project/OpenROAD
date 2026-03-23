include("openroad")

set(TEST_LIBS
  web
  odb
  utl_lib
  lodepng
  tst
  GTest::gtest
  GTest::gtest_main
)

add_executable(TestTileGenerator TestTileGenerator.cpp)

target_link_libraries(TestTileGenerator ${TEST_LIBS})

target_include_directories(TestTileGenerator
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestTileGenerator
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestRequestHandler TestRequestHandler.cpp)

target_link_libraries(TestRequestHandler ${TEST_LIBS} gui_descriptors)

target_include_directories(TestRequestHandler
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestRequestHandler
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test
  TestTileGenerator
  TestRequestHandler
)

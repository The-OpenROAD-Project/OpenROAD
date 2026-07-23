include("openroad")

set(TEST_LIBS
  web
  odb
  utl_lib
  lodepng
  tst
  gui_descriptors
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

target_link_libraries(TestRequestHandler ${TEST_LIBS})

target_include_directories(TestRequestHandler
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestRequestHandler
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestSaveReport TestSaveReport.cpp)

target_link_libraries(TestSaveReport ${TEST_LIBS})

target_include_directories(TestSaveReport
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestSaveReport
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestDebugGraphics TestDebugGraphics.cpp)

target_link_libraries(TestDebugGraphics ${TEST_LIBS})

target_include_directories(TestDebugGraphics
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestDebugGraphics
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestGlyphCache TestGlyphCache.cpp)

target_link_libraries(TestGlyphCache ${TEST_LIBS})

target_include_directories(TestGlyphCache
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestGlyphCache
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestClockTreeReport TestClockTreeReport.cpp)

target_link_libraries(TestClockTreeReport ${TEST_LIBS})

target_include_directories(TestClockTreeReport
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestClockTreeReport
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestSaveImage TestSaveImage.cpp)

target_link_libraries(TestSaveImage ${TEST_LIBS})

target_include_directories(TestSaveImage
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestSaveImage
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestSnap TestSnap.cpp)

target_link_libraries(TestSnap ${TEST_LIBS})

target_include_directories(TestSnap
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestSnap
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestTimingCone TestTimingCone.cpp)

target_link_libraries(TestTimingCone ${TEST_LIBS})

target_include_directories(TestTimingCone
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestTimingCone
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestCharts TestCharts.cpp)

target_link_libraries(TestCharts ${TEST_LIBS})

target_include_directories(TestCharts
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestCharts
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestLabels TestLabels.cpp)

target_link_libraries(TestLabels ${TEST_LIBS})

target_include_directories(TestLabels
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestLabels
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test
  TestTileGenerator
  TestRequestHandler
  TestSaveReport
  TestDebugGraphics
  TestGlyphCache
  TestClockTreeReport
  TestSaveImage
  TestSnap
  TestTimingCone
  TestCharts
  TestLabels
)

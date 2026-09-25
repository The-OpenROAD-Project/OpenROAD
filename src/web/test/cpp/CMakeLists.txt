include("openroad")

set(TEST_LIBS
  web
  odb
  utl_lib
  lodepng
  tst
  gui_core
  GTest::gtest
  GTest::gtest_main
)

# gui_core in TEST_LIBS above carries everything these tests reach: the
# descriptors, the heatmap core, the rendering API and Gui's dispatch spine.
# Linking the Qt gui library instead would drag in the whole application.

set(WEB_TESTS
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
  TestSaveDisplayControls
  TestGif
)

foreach(test IN LISTS WEB_TESTS)
  add_executable(${test} ${test}.cpp)

  target_link_libraries(${test} ${TEST_LIBS})

  target_include_directories(${test}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../../src
  )

  gtest_discover_tests(${test}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
  )
endforeach()

add_dependencies(build_and_test ${WEB_TESTS})

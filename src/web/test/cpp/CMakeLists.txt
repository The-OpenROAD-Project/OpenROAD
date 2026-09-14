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

# Every test compiles in the headless gui stub, mirroring the Bazel targets,
# which all depend on //src/gui:gui_stub.  gui_core carries the descriptors,
# the heatmap core and the rendering API but no Gui dispatch of its own, so
# anything that reaches gui::Gui::{get,enabled,selection,setHeadlessViewer,
# setChartFactory} needs this to link.  Linking the Qt gui library instead
# would drag in the whole application.
#
# The stub's initGui() calls DescriptorRegistry::initDescriptors(); the
# descriptor implementations behind it come from gui_core.
set(GUI_STUB_SRCS
  ${OPENROAD_HOME}/src/gui/src/stub.cpp
)

set(GUI_STUB_INCLUDES
  ${OPENROAD_HOME}/src/gui/src
  ${TCL_INCLUDE_PATH}
)

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
  add_executable(${test} ${test}.cpp ${GUI_STUB_SRCS})

  target_link_libraries(${test} ${TEST_LIBS})

  target_include_directories(${test}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../../src
      ${GUI_STUB_INCLUDES}
  )

  gtest_discover_tests(${test}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
  )
endforeach()

add_dependencies(build_and_test ${WEB_TESTS})

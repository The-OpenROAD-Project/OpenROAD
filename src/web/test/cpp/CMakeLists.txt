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

# A test that constructs a WebServer pulls in web.cpp.o, which references
# WebServer::initLogger() in web_serve.cpp, which in turn references
# gui::Gui::{get,setHeadlessViewer,setChartFactory}.  Compile the headless gui
# stub into those tests -- the Qt gui library would drag in the whole
# application -- mirroring the Bazel targets' gui_stub dependency.  `web`
# itself links only gui_descriptors, so nothing else defines these symbols and
# there is no clash in either a Qt or a non-Qt build.
#
# The stub's initGui() calls DescriptorRegistry::initDescriptors(), so the
# descriptor implementations come along too.  gui_descriptors deliberately
# holds only the registry, to keep these out of the web library; they compile
# without Qt here, as they do in the CLI-only gui library.
set(GUI_STUB_SRCS
  ${OPENROAD_HOME}/src/gui/src/stub.cpp
  ${OPENROAD_HOME}/src/gui/src/init_descriptors.cpp
  ${OPENROAD_HOME}/src/gui/src/dbDescriptors.cpp
  ${OPENROAD_HOME}/src/gui/src/staDescriptors.cpp
)

set(GUI_STUB_INCLUDES
  ${OPENROAD_HOME}/src/gui/src
  ${TCL_INCLUDE_PATH}
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

add_executable(TestSaveReport TestSaveReport.cpp ${GUI_STUB_SRCS})

target_link_libraries(TestSaveReport ${TEST_LIBS})

target_include_directories(TestSaveReport
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${GUI_STUB_INCLUDES}
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

add_executable(TestSaveDisplayControls
  TestSaveDisplayControls.cpp
  ${GUI_STUB_SRCS}
)

target_link_libraries(TestSaveDisplayControls ${TEST_LIBS})

target_include_directories(TestSaveDisplayControls
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${GUI_STUB_INCLUDES}
)

gtest_discover_tests(TestSaveDisplayControls
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestGif TestGif.cpp ${GUI_STUB_SRCS})

target_link_libraries(TestGif ${TEST_LIBS})

target_include_directories(TestGif
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${GUI_STUB_INCLUDES}
)

gtest_discover_tests(TestGif
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestWebAssets TestWebAssets.cpp)

target_link_libraries(TestWebAssets ${TEST_LIBS})

target_include_directories(TestWebAssets
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

gtest_discover_tests(TestWebAssets
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(TestCssInliner TestCssInliner.cpp ${GUI_STUB_SRCS})

target_link_libraries(TestCssInliner ${TEST_LIBS})

target_include_directories(TestCssInliner
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${GUI_STUB_INCLUDES}
)

gtest_discover_tests(TestCssInliner
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
  TestSaveDisplayControls
  TestGif
  TestWebAssets
  TestCssInliner
)

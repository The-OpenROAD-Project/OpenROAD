find_package(Boost)
include("openroad")


set(DFT_HOME ${PROJECT_SOURCE_DIR})

set(TEST_LIBS
  dbSta
  dft)

add_executable(TestScanArchitectHeuristic TestScanArchitectHeuristic.cpp ScanCellMock.cpp)
target_link_libraries(TestScanArchitectHeuristic ${TEST_LIBS})
target_include_directories(TestScanArchitectHeuristic
  PRIVATE
  ${DFT_HOME}/src
)


add_executable(TestScanArchitect TestScanArchitect.cpp)
target_link_libraries(TestScanArchitect ${TEST_LIBS})
target_include_directories(TestScanArchitect
  PRIVATE
  ${DFT_HOME}/src
)

find_package(Boost)
include("openroad")


set(DFT_HOME ${PROJECT_SOURCE_DIR})

set(TEST_LIBS
  utl
  dbSta
  dft_cells_lib
  dft_config_lib
  dft_architect_lib
  dft_clock_domain_lib
)

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

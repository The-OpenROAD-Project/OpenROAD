include("openroad")

set(TEST_LIBS
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  odb
  dft_base_scan_cell_lib
  dft_config_lib
  dft_architect_lib
  dft_optimizer_lib
  dft_clock_domain_lib
  dft_utils_scan_pin_lib
)

add_executable(TestScanArchitect TestScanArchitect.cpp)
target_link_libraries(TestScanArchitect ${TEST_LIBS})
gtest_discover_tests(TestScanArchitect WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TestScanArchitectHeuristic TestScanArchitectHeuristic.cpp ScanCellMock.cpp)
target_link_libraries(TestScanArchitectHeuristic ${TEST_LIBS})
gtest_discover_tests(TestScanArchitectHeuristic WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TestScanArchitectMultibit TestScanArchitectMultibit.cpp)
target_link_libraries(TestScanArchitectMultibit ${TEST_LIBS})
gtest_discover_tests(TestScanArchitectMultibit WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TestScanArchitectMaxChains TestScanArchitectMaxChains.cpp)
target_link_libraries(TestScanArchitectMaxChains ${TEST_LIBS})
gtest_discover_tests(TestScanArchitectMaxChains WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TestScanReplaceMultibit TestScanReplaceMultibit.cpp)
target_link_libraries(TestScanReplaceMultibit
  GTest::gtest
  GTest::gtest_main
  dft
  tst
)
gtest_discover_tests(TestScanReplaceMultibit WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(TestScanOptimizer TestScanOptimizer.cpp)
target_link_libraries(TestScanOptimizer ${TEST_LIBS})
gtest_discover_tests(TestScanOptimizer WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(build_and_test
  TestScanArchitect
  TestScanArchitectHeuristic
  TestScanArchitectMultibit
  TestScanArchitectMaxChains
  TestScanReplaceMultibit
  TestScanOptimizer
)

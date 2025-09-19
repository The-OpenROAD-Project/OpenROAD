include(openroad)

set(TEST_LIBS
  odb
  zutil
  lef
  defin
  defout
  lefin
  lefout
  cdl
  ${TCL_LIBRARY}
  Boost::boost
  utl_lib
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  odb_test_helper
)

add_executable(TestScanChain TestScanChain.cpp)
target_link_libraries(TestScanChain ${TEST_LIBS})

# Skip TestScanChain tests since they are called via cpp_tests.tcl, which is
# preferred since BASE_DIR is set and the output files are written to the
# results dir. Running TestScanChain outside of cpp_tests.tcl results in the
# output files being written in the current working directory
#
gtest_discover_tests(TestScanChain
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}
    TEST_FILTER "-TestScanChain.CreateScanChain*"
)

add_dependencies(build_and_test TestScanChain)

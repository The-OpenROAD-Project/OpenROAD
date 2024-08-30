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

gtest_discover_tests(TestScanChain WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(build_and_test TestScanChain)

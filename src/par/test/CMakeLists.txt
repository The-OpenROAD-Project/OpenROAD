or_integration_tests(
  "par"
  TESTS
    partition_gcd
    read_part
    write_artnet
  PASSFAIL_TESTS
    ilp_fixed_vertices
)

add_executable(TestILPFixedVertices cpp/TestILPFixedVertices.cpp)
target_include_directories(TestILPFixedVertices PRIVATE ${OPENROAD_HOME})
target_link_libraries(TestILPFixedVertices
  par_lib
  utl_lib
  GTest::gtest
  GTest::gtest_main
)
gtest_discover_tests(TestILPFixedVertices
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)
add_dependencies(build_and_test TestILPFixedVertices)


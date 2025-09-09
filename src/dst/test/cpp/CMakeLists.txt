set(TEST_LIBS
    dst
    dl
    ${TCL_LIBRARY}
)

set(DST_HOME ${PROJECT_SOURCE_DIR})

add_executable(TestWorker TestWorker.cc stubs.cpp)
add_executable(TestBalancer TestBalancer.cc stubs.cpp)
add_executable(TestDistributed TestDistributed.cc stubs.cpp)

target_link_libraries(TestWorker ${TEST_LIBS})
target_link_libraries(TestBalancer ${TEST_LIBS})
target_link_libraries(TestDistributed ${TEST_LIBS})

target_include_directories(TestWorker
  PRIVATE
  ${DST_HOME}/src
  ${OPENROAD_HOME}/include
)
target_include_directories(TestBalancer
  PRIVATE
  ${DST_HOME}/src
  ${OPENROAD_HOME}/include
)
target_include_directories(TestDistributed
  PRIVATE
  ${DST_HOME}/src
  ${OPENROAD_HOME}/include
)

add_test(
  NAME "dst.TestWorker"
  COMMAND TestWorker
)

add_test(
  NAME "dst.TestBalancer"
  COMMAND TestBalancer
)

# This test case appears to have an internal race condition
#add_test(
#  NAME "dst.TestDistributed"
#  COMMAND TestDistributed
#)

add_dependencies(build_and_test 
  TestWorker
  TestBalancer
  TestDistributed
)

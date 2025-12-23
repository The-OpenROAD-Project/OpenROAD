include("openroad")

set(TEST_LIBS
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  utl_base
)

add_executable(TestCFileUtils TestCFileUtils.cpp)

target_link_libraries(TestCFileUtils ${TEST_LIBS})

gtest_discover_tests(TestCFileUtils
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(TestMetrics TestMetrics.cpp)

target_link_libraries(TestMetrics ${TEST_LIBS})

gtest_discover_tests(TestMetrics
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(TestSuppressStdout TestSuppressStdout.cpp)
target_link_libraries(TestSuppressStdout ${TEST_LIBS})

gtest_discover_tests(TestSuppressStdout
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_executable(TestBackTraceOnDebugError TestBackTraceOnErrorWithDebug.cpp)
target_link_libraries(TestBackTraceOnDebugError ${TEST_LIBS})
target_compile_options(TestBackTraceOnDebugError PRIVATE -g -fno-omit-frame-pointer)

gtest_discover_tests(TestBackTraceOnDebugError
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(TestCriticalPrintsBacktrace TestLoggerCriticalPrintsBackTrace.cpp)
target_link_libraries(TestCriticalPrintsBacktrace ${TEST_LIBS})
target_compile_options(TestCriticalPrintsBacktrace PRIVATE -g)

gtest_discover_tests(TestCriticalPrintsBacktrace 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(UNIX)
    target_link_options(TestCriticalPrintsBacktrace PRIVATE -rdynamic)
    target_link_options(TestBackTraceOnDebugError PRIVATE -rdynamic)
endif()

add_dependencies(build_and_test
  TestMetrics
  TestCFileUtils
  TestCriticalPrintsBacktrace
  TestBackTraceOnDebugError
  TestSuppressStdout
)
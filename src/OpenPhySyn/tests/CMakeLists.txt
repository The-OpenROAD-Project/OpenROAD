cmake_minimum_required(VERSION 3.8.2)

set(TEST_MAIN unit_tests)   # Default name for test executable (change if you wish).
set(TEST_RUNNER_PARAMS "")  # Any arguemnts to feed the test runner (change as needed).

set(PSN_TEST_WRAP ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Psn_wrap_test.cc)
set_source_files_properties(${PSN_TEST_WRAP} PROPERTIES COMPILE_FLAGS ${SWIG_SOURCE_COMPILE_FLAGS})


add_executable(${TEST_MAIN} ${PSN_TESTFILES} ${PSN_SOURCES_NO_MAIN_NO_WRAP} ${PSN_TEST_WRAP})

add_custom_command(OUTPUT ${PSN_TEST_WRAP}
  COMMAND ${SWIG_EXECUTABLE} ${SWIG_FLAGS} ${SWIG_PSN_LIB_FLAGS} -tcl8 -c++ -namespace -prefix psn -I${PSN_HOME}/src -o ${PSN_TEST_WRAP} ${PSN_SWIG_FILES}
  WORKING_DIRECTORY ${PSN_HOME}
  DEPENDS ${PSN_SWIG_FILES}
)

if (${OPENROAD_BUILD})
set_source_files_properties(
  ${OPENSTA_UTIL_TCL_INIT}
  PROPERTIES GENERATED TRUE
  )
target_link_libraries(${TEST_MAIN} PUBLIC OpenPhySyn)
endif()

target_link_libraries(${TEST_MAIN} PRIVATE doctest PUBLIC ${PUBLIC_EXEC_LIBRARIES} ${PUBLIC_LIBRARIES})
set_target_properties(${TEST_MAIN} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
# target_set_warnings(${TEST_MAIN} ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Set warnings (if needed).

if (${OPENROAD_BUILD})
add_dependencies(${TEST_MAIN} opensta-tcl-init)
else()
set_target_properties(${TEST_MAIN} PROPERTIES 
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES 
    CXX_EXTENSIONS NO
)
endif()

add_test(
    NAME ${LIBRARY_NAME}.${TEST_MAIN}
    WORKING_DIRECTORY ${PSN_HOME}/build
    COMMAND ${PSN_HOME}/build/${TEST_MAIN} ${TEST_RUNNER_PARAMS}
)
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES 
  ${PSN_TEST_WRAP}
)

# Adds a 'coverage' target.
include(CodeCoverage)


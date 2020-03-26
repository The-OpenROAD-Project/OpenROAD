cmake_minimum_required(VERSION 3.8.2)

set(TEST_MAIN unit_tests)   # Default name for test executable (change if you wish).
set(TEST_RUNNER_PARAMS "")  # Any arguemnts to feed the test runner (change as needed).

set(PSN_TEST_WRAP ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Psn_wrap_test.cc)
set_source_files_properties(${PSN_TEST_WRAP} PROPERTIES COMPILE_FLAGS ${SWIG_SOURCE_COMPILE_FLAGS})

# Generate TCL init file for test initialization.
set(OPENPHYSYN_OPENROAD_TCL_FILES
  ${OPENSTA_HOME}/tcl/Util.tcl
  ${OPENROAD_HOME}/src/OpenRoad.tcl
  ${ICEWALL_HOME}/src/ICeWall.tcl
  ${PDNGEN_HOME}/src/PdnGen.tcl
  )

set(OPENPHYSYN_OPENROAD_TCL ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/OpenRoadTclInitVar.cc)

add_custom_target(openroad_tcl
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${OPENPHYSYN_OPENROAD_TCL} openroad_tcl_inits ${OPENPHYSYN_OPENROAD_TCL_FILES}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENPHYSYN_OPENROAD_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
)

add_executable(${TEST_MAIN} ${PSN_TESTFILES} ${PSN_SOURCES_NO_MAIN_NO_WRAP} ${OPENPHYSYN_OPENROAD_TCL} ${PSN_TEST_WRAP})

set_source_files_properties(${OPENPHYSYN_OPENROAD_TCL} PROPERTIES GENERATED true)
add_dependencies(${TEST_MAIN} openroad_tcl)

add_custom_command(OUTPUT ${PSN_TEST_WRAP}
  COMMAND ${SWIG_EXECUTABLE} ${SWIG_FLAGS} ${SWIG_PSN_LIB_FLAGS} -tcl8 -c++ -namespace -prefix psn -I${PSN_HOME}/src -o ${PSN_TEST_WRAP} ${PSN_SWIG_FILES}
  WORKING_DIRECTORY ${PSN_HOME}
  DEPENDS ${PSN_SWIG_FILES}
)

# target_link_libraries(${TEST_MAIN} )



target_link_libraries(${TEST_MAIN} PUBLIC OpenPhySyn)

target_link_libraries(${TEST_MAIN} PRIVATE doctest PUBLIC ${PUBLIC_EXEC_LIBRARIES} ${PUBLIC_LIBRARIES})
set_target_properties(${TEST_MAIN} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})


add_dependencies(${TEST_MAIN} opensta-tcl-init)



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


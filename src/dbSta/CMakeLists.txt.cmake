# Core LEF Checker Library
add_library(lef_checker
  LefChecker.cc
)

target_include_directories(lef_checker
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${OPENROAD_INCLUDE_DIRS}
)

target_link_libraries(lef_checker
  PUBLIC
  odb
)

# Tcl Interface
configure_file(
  LefCheckerTcl.i
  ${CMAKE_CURRENT_BINARY_DIR}/LefCheckerTcl.i
  COPYONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/LefCheckerTcl.i
  DESTINATION ${TCL_INSTALL_DIR}
)

# Python Bindings (Optional)
if(SWIG_FOUND AND PYTHON3_FOUND)
  swig_add_library(lef_checker_python
    LANGUAGE python
    SOURCES ${CMAKE_SOURCE_DIR}/src/odb/python/lef_checker.i
  )
  
  target_include_directories(lef_checker_python
    PRIVATE
    ${PYTHON3_INCLUDE_DIRS}
  )
  
  target_link_libraries(lef_checker_python
    lef_checker
    ${PYTHON3_LIBRARIES}
  )
  
  install(
    TARGETS lef_checker_python
    DESTINATION ${PYTHON3_SITE_PACKAGES}
  )
endif()

# Testing
if(BUILD_TESTING)
  add_executable(test_lef_checker
    test/LefCheckerTest.cc
  )
  
  target_link_libraries(test_lef_checker
    lef_checker
    gtest
  )
  
  add_test(
    NAME test_lef_checker
    COMMAND test_lef_checker
  )
endif()

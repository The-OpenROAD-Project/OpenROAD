# CMake generated Testfile for 
# Source directory: /home/memzfs_projects/MLBuf_extension/OR_latest/src/gui/test
# Build directory: /home/memzfs_projects/MLBuf_extension/OR_latest/build/src/gui/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(gui.supported.tcl "/usr/bin/bash" "/home/memzfs_projects/MLBuf_extension/OR_latest/test/regression_test.sh")
set_tests_properties(gui.supported.tcl PROPERTIES  ENVIRONMENT "OPENROAD_EXE=/home/memzfs_projects/MLBuf_extension/OR_latest/build/bin/openroad;TEST_NAME=supported;TEST_EXT=tcl;TEST_TYPE=tcl;TEST_CHECK_LOG=True;TEST_CHECK_PASSFAIL=False" LABELS "IntegrationTest tcl gui log_compare" WORKING_DIRECTORY "/home/memzfs_projects/MLBuf_extension/OR_latest/src/gui/test" _BACKTRACE_TRIPLES "/home/memzfs_projects/MLBuf_extension/OR_latest/src/cmake/testing.cmake;19;add_test;/home/memzfs_projects/MLBuf_extension/OR_latest/src/cmake/testing.cmake;86;or_integration_test_single;/home/memzfs_projects/MLBuf_extension/OR_latest/src/gui/test/CMakeLists.txt;1;or_integration_tests;/home/memzfs_projects/MLBuf_extension/OR_latest/src/gui/test/CMakeLists.txt;0;")

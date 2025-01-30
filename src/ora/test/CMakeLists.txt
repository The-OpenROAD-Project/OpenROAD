add_test(
  NAME mockApiServer_setup
  COMMAND sh "-c" 
    "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mockApiServer.py > server.log 2>&1 & echo $! > server.pid;
     for i in $(seq 1 5); do
       if grep -q 'SERVER_READY:' server.log; then
         exit 0;
       fi;
       sleep 1;
     done;
     echo 'Server failed to start'; exit 1"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(mockApiServer_setup PROPERTIES
  FIXTURES_SETUP "mockapi_fixture"
  TIMEOUT 5
  DEPENDS mockApiServer_preclean
)

add_test(
  NAME mockApiServer_teardown
  COMMAND sh -c "kill $(cat server.pid) || true; rm -f server.pid server.log"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(mockApiServer_teardown PROPERTIES
  FIXTURES_CLEANUP "mockapi_fixture"
)

or_integration_tests(
  "ora"
  TESTS
    ora_init1
    askbot1
)

set_tests_properties("ora.ora_init1.tcl" PROPERTIES
  FIXTURES_SETUP "ora_init_fixture"
)

foreach(TEST_NAME IN ITEMS askbot1)
  set_tests_properties("ora.${TEST_NAME}.tcl" PROPERTIES
    FIXTURES_REQUIRED "mockapi_fixture;ora_init_fixture"
  )
endforeach()

foreach(TEST_NAME IN ITEMS ora_init1 askbot1)
  set_tests_properties("ora.${TEST_NAME}.tcl" PROPERTIES
    FIXTURES_REQUIRED "mockapi_fixture"
  )
endforeach()
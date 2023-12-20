find_program (BASH_PROGRAM bash REQUIRED)

function(gpl_test test_name)
  add_test (
    NAME gpl.${test_name} 
    COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/regression ${test_name}
  )
  set_tests_properties(gpl.${test_name}  PROPERTIES LABELS "IntegrationTest")
endfunction()

set(TEST_NAMES
simple01
simple01-obs
simple01-td
simple01-td-tune
simple01-uniform
simple01-ref
simple01-skip-io
simple02
simple03
simple04
simple05
simple06
simple07
simple08
simple09
core01
ar01
ar02
incremental01
incremental02
error01
diverge01
density01
convergence01
nograd01
clust01
clust02
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  gpl_test(${TEST_NAME})
endforeach()

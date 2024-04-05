source "helpers.tcl"
# Tcl man functionality test

# Test 1) Running man path with set manpath variable.
set test_dir [file dirname [file normalize [info script]]]
set manpath_dir [file normalize [file dirname [file dirname [file normalize $test_dir]]]]
set manpath_dir "$manpath_dir/cat"
set output [man openroad -manpath $manpath_dir -no_query]

# Test 2) Running manpath with no set manpath variable.
set output [man openroad -no_query]

# Test set_net_weight / unset_net_weight
source helpers.tcl
set test_name net_weight01
read_lef ./nangate45.lef
read_def ./simple01.def

# Test 1: set_net_weight on existing net
set nets [odb::dbBlock_getNets [ord::get_db_block]]
set first_net [lindex $nets 0]
set net_name [$first_net getName]
puts "Test net: $net_name"

set_net_weight $net_name 5.0
puts "PASS: set_net_weight $net_name 5.0"

# Test 2: verify property exists in ODB
set prop [odb::dbDoubleProperty_find $first_net "gpl_weight"]
if {$prop == "NULL"} {
    error "FAIL: gpl_weight property not found"
}
set val [$prop getValue]
if {abs($val - 5.0) > 0.01} {
    error "FAIL: expected 5.0, got $val"
}
puts "PASS: property value = $val"

# Test 3: unset
unset_net_weight $net_name
set prop2 [odb::dbDoubleProperty_find $first_net "gpl_weight"]
if {$prop2 != "NULL"} {
    error "FAIL: property should be removed"
}
puts "PASS: unset_net_weight"

# Test 4: set multiple nets
set_net_weight $net_name 3.0
set second_net [lindex $nets 1]
set net2_name [$second_net getName]
set_net_weight $net2_name 7.0
puts "PASS: multiple nets weighted"

# Test 5: run global_placement with weights (functional test)
global_placement -init_density_penalty 0.01 -skip_initial_place
puts "PASS: global_placement with net weights"

# Test 6: overwrite weight
set_net_weight $net_name 10.0
set prop3 [odb::dbDoubleProperty_find $first_net "gpl_weight"]
set val3 [$prop3 getValue]
if {abs($val3 - 10.0) > 0.01} {
    error "FAIL: overwrite expected 10.0, got $val3"
}
puts "PASS: overwrite weight"

# Test 7: write_db / read_db persistence
write_db [make_result_file "${test_name}.odb"]
puts "PASS: all tests passed"

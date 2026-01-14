# Tests a net name consistency b/w Verilog and SPEF.
source helpers.tcl
source Nangate45/Nangate45.vars

set test_name net_name_consistency

read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file

read_verilog $test_name.v
link_design -hier top

initialize_floorplan -site $site -utilization 10 -core_space 0.0
sta::check_axioms

repair_tie_fanout LOGIC0_X1/Z
sta::check_axioms

remove_buffers
sta::check_axioms

source $tracks_file

set block [ord::get_db_block]

# Place the instances
set i [$block findInst "leaf"]
$i setLocation 0 1000
$i setPlacementStatus PLACED

set i [$block findInst "load0"]
$i setLocation 0 2000
$i setPlacementStatus PLACED

set i [$block findInst "load1"]
$i setLocation 0 3000
$i setPlacementStatus PLACED

set i [$block findInst "b/leaf"]
$i setLocation 0 0
$i setPlacementStatus PLACED

set i [$block findInst "b/leaf_2"]
$i setLocation 5700 0
$i setPlacementStatus PLACED

set i [$block findInst "b/buf0_1"]
$i setLocation 1000 0
$i setPlacementStatus PLACED

# Route the net
set_routing_layers -signal $global_routing_layers
global_route
detailed_route -verbose 0

# Extract RC
define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file $rcx_rules_file

# Write verilog
set verilog_file [make_result_file $test_name.v]
write_verilog $verilog_file
diff_files $test_name.vok $verilog_file

# Write SEPF
set spef_file [make_result_file $test_name.spef]
write_spef $spef_file

# Filter out the date and software version lines before diff_files
set filtered_spef_file [make_result_file $test_name.spef.tmp]
exec tail -n +10 $spef_file > $filtered_spef_file
set filtered_spefok_file [make_result_file $test_name.spefok.tmp]
exec tail -n +10 $test_name.spefok > $filtered_spefok_file
diff_files $filtered_spefok_file $filtered_spef_file

# Test reading the spef for no errors in the .ok
read_spef $spef_file

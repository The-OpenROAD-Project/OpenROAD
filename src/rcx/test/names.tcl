# Tests the handling of hierarchical and escaped names in read/write spef
# This test starts with read_verilog to make sure that the names generated
# in odb are what are generated during verilog parsing and not from DEF.
source helpers.tcl
source Nangate45/Nangate45.vars

read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file

read_verilog names.v
link_design top

initialize_floorplan -site $site -utilization 10 -core_space 0.0

source $tracks_file

set block [ord::get_db_block]

# Place the instances
set i [$block findInst "disolved_top\\/leaf"]
$i setLocation 0 0
$i setPlacementStatus PLACED

set i [$block findInst "b/disolved_block\\/leaf"]
$i setLocation 5700 0
$i setPlacementStatus PLACED

# Route the net
set_routing_layers -signal $global_routing_layers
global_route
detailed_route -verbose 0

# Extract  
define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file $rcx_rules_file

set spef_file [make_result_file names.spef]
write_spef $spef_file

# Test reading the spef for no errors in the .ok
read_spef $spef_file

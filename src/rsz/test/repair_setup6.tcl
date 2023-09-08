# repair_timing -setup with global route parasitics
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
initialize_floorplan -die_area "0 0 40 1200" \
  -core_area "0 0 40 1200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
source $tracks_file

create_clock -period 0.3 clk
set_propagated_clock clk

detailed_placement

source Nangate45/Nangate45.rc
# Intentionaly reduce RC so there is a large discrepancy between
# placement and global route parasitics.
set_wire_rc -resistance 0.0001 -capacitance 0.00001
set_routing_layers -signal $global_routing_layers
global_route
estimate_parasitics -global_routing

report_worst_slack -max
write_verilog_for_eqy repair_setup6 before "None"
repair_timing -setup
run_equivalence_test repair_setup6 ./Nangate45/work_around_yosys/ "None"
report_worst_slack -max

# check slacks with fresh parasitics
global_route
estimate_parasitics -global_routing
report_worst_slack -max

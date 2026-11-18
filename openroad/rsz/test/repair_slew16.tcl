# repair_slew1 with global route parasitics
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
set def_file [make_result_file "repair_slew16.def"]
write_hi_fanout_def $def_file 20
read_def $def_file

initialize_floorplan -die_area "0 0 88 88" \
  -core_area "0 0 88 88" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
source $tracks_file

create_clock -period 1 clk1

detailed_placement

set_routing_layers -signal $global_routing_layers
global_route
estimate_parasitics -global_routing

set_max_transition .05 [current_design]

report_check_types -max_slew -max_cap -max_fanout

repair_design

report_check_types -max_slew -max_cap -max_fanout

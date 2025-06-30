# repair_wire1 with global route parasitics
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

initialize_floorplan -die_area "0 0 2100 200" \
  -core_area "0 0 2100 200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
source $tracks_file

detailed_placement

set_routing_layers -signal $global_routing_layers
global_route
estimate_parasitics -global_routing

report_checks -unconstrained -fields {input slew cap} -to out1
report_long_wires 2

repair_design -max_wire_length 800

report_checks -unconstrained -fields {input slew cap} -to out1
report_long_wires 2

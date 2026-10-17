# estimate_parasitics -global_routing -spef_file must write *CAP for pin nodes
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def est_rc2.def

set_routing_layers -signal metal2-metal10

global_route
set spef_file [make_result_file est_rc_spef_pin_cap.spef]
estimate_parasitics -global_routing -spef_file $spef_file

diff_files est_rc_spef_pin_cap.spefok $spef_file

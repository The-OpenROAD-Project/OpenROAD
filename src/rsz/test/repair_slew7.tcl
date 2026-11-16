# repair_design max_slew 2 corners
source "helpers.tcl"
read_liberty Nangate45/Nangate45_slow.lib
read_liberty Nangate45/Nangate45_fast.lib
define_scene slow -liberty Nangate45/Nangate45_slow.lib
define_scene fast -liberty Nangate45/Nangate45_fast.lib
set_scene slow
read_lef Nangate45/Nangate45.lef
read_def repair_slew5.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
set_load 5 [get_net u1zn]

report_checks -unconstrained -fields {slew capacitance}
report_check_types -max_slew -max_capacitance

repair_design

report_checks -unconstrained -fields {slew capacitance}
report_check_types -max_slew -max_capacitance

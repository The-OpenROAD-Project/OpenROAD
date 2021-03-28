# repair_design max_slew violation from resizing loads
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew5.def

set_wire_rc -layer metal3
estimate_parasitics -placement
# under the cap limit but high enough for slew violation
set_load 50 [get_net u2zn]
set_load 50 [get_net u3zn]
set_load 50 [get_net u4zn]
set_load 50 [get_net u5zn]
set_load 50 [get_net u6zn]
set_load 50 [get_net u7zn]

set_max_transition .15 [current_design]
report_net -connections -verbose u1/ZN
report_check_types -max_slew -max_cap -max_fanout -violators

repair_design

report_check_types -max_slew -max_cap -max_fanout

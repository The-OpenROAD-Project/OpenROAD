# repair_design max_slew liberty pin max_transition
source "helpers.tcl"
source "hi_fanout.tcl"

# DFF_X1/Q max_transition
read_liberty repair_slew2.lib
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
set def_file [make_result_file "repair_slew2.def"]
write_hi_fanout_def $def_file 20
read_def $def_file
create_clock -period 1 clk1

set_wire_rc -layer metal3
estimate_parasitics -placement

report_check_types -max_slew -violators
repair_design
report_check_types -max_slew -violators

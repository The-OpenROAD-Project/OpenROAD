# buffer_ports -input hi fanout reset net (no req time) -> max slew violation
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [make_result_file "buffer_ports4.def"]
write_hi_fanout_input_def $def_filename 250

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 1 clk1

set_wire_rc -layer metal3
estimate_parasitics -placement

buffer_ports -inputs

report_check_types -max_slew

repair_design

report_check_types -max_slew

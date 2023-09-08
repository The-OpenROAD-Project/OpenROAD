# repair_design max_fanout
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [make_result_file "repair_fanout1.def"]
write_hi_fanout_def $def_filename 35

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 10 clk1
set_max_fanout 10 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_check_types -max_fanout
write_verilog_for_eqy repair_fanout1 before "None"
repair_design
run_equivalence_test repair_fanout1 ./Nangate45/work_around_yosys/ "None"
report_check_types -max_fanout

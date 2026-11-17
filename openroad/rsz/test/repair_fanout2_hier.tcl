# repair_design max_fanout
source "helpers.tcl"
source Nangate45/Nangate45.vars
source "hi_fanout.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_fanout2_hier.v

link_design hi_fanout -hier


# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize repair_fanout2_hier.def

create_clock -period 10 clk1
set_max_fanout 10 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_check_types -max_fanout

repair_design
report_check_types -max_fanout
set verilog_file [make_result_file repair_fanout2_hier_out.v]
write_verilog $verilog_file
diff_files $verilog_file repair_fanout2_hier_out.vok

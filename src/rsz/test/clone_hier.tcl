# repair_design max_fanout
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def clone_hier.def

create_clock -period 0.1 clk1
set_driving_cell -lib_cell BUF_X1 [all_inputs]
# Make sure the fanout fixes are not applied so we can test
# gate cloning fixes.
set_max_fanout 200 [current_design]

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
set_wire_rc -layer metal2
estimate_parasitics -placement

# Skip repair_design for now since this is a test for gate
# cloning

set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

estimate_parasitics -placement


# Repair the high fanout net hopefully with gate cloning code.
report_worst_slack -max
repair_timing -setup -repair_tns 100 -verbose
report_worst_slack -max
set verilog_file [make_result_file clone_hier_out.v]
write_verilog $verilog_file
diff_files $verilog_file clone_hier_out.vok

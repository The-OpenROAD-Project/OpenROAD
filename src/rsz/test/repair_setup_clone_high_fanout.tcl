# Probe CloneMove vs CloneGenerator/Candidate behavior on high-fanout net.
source "helpers.tcl"
source "hi_fanout.tcl"
set def_filename [make_result_file "move_diff_clone.def"]
write_clone_test_def $def_filename NAND2_X4 30
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 0.1 clk1
set_driving_cell -lib_cell BUF_X1 [all_inputs]
set_max_fanout 200 [current_design]
source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
set_wire_rc -layer metal2
estimate_parasitics -placement
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use
estimate_parasitics -placement
report_worst_slack -max
repair_timing -setup -sequence "clone" -skip_last_gasp -repair_tns 100 -max_passes 20 -verbose
report_worst_slack -max
report_tns -digits 3

# repair_design max_fanout
source "helpers.tcl"
source "hi_fanout.tcl"

set def_filename [make_result_file "repair_fanout8.def"]
# Gates we want to eventually NAND2_X4, NAND3_X4, NAND4_X4

write_clone_test_def $def_filename NAND2_X4 150

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
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
set_wire_rc -clock  -layer $wire_rc_layer_clk
set_dont_use $dont_use

estimate_parasitics -placement

report_worst_slack -max
write_verilog_for_eqy repair_fanout8 before "None"
repair_timing -setup -repair_tns 100 -skip_gate_cloning -verbose
run_equivalence_test repair_fanout8 ./Nangate45/work_around_yosys/ "None"
report_worst_slack -max


# repair_timing -setup r1/Q 5 loads
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_checks -fields input -digits 3

write_verilog_for_eqy repair_setup1 before "None"
repair_timing -setup
run_equivalence_test repair_setup1 ./Nangate45/work_around_yosys/ "None"
report_checks -fields input -digits 3

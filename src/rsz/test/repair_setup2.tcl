# repair_timing -setup combinational path
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock  -layer metal5
estimate_parasitics -placement

report_worst_slack
write_verilog_for_eqy repair_setup2 before "None"
repair_design
repair_timing -setup -verbose
run_equivalence_test repair_setup2 ./Nangate45/work_around_yosys/ "None"
report_worst_slack

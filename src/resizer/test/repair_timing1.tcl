# repair_timing
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_timing1.def
create_clock -period 1 clk
set_wire_rc -layer metal3
estimate_parasitics -placement
report_checks -fields input -digits 3
#repair_timing -buffer_cell "BUF_X1"
#report_checks -fields input -digits 3

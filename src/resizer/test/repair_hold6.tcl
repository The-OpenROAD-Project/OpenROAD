# repair_hold_violations repair_hold5 with -allow_setup_violations
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold4.def

create_clock -period 2 clk
set_propagated_clock clk
set_min_delay -from r1/CK -to r2/D 0.4
set_max_delay -from r1/CK -to r2/D 0.2

set_wire_rc -layer metal1
estimate_parasitics -placement

report_checks -path_delay min_max -format full_clock -digits 3

repair_hold_violations -buffer_cell BUF_X1 -allow_setup_violations

report_checks -path_delay min_max -format full_clock -digits 3

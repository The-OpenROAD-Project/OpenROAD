# repair tie hi/low net
source "helpers.tcl"
source "resizer_helpers.tcl"
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_tie8.def

repair_tie_fanout sky130_fd_sc_hd__conb_1/LO
repair_tie_fanout sky130_fd_sc_hd__conb_1/HI
check_ties LOGIC1_X1
report_ties LOGIC1_X1


# repair_design max load slew with long wire to output
# in1--u1--------------------------out1
#          500  |--u2              2000
source "helpers.tcl"
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_slew14.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement

report_check_types -max_slew
repair_design
report_check_types -max_slew

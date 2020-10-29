# resize
source helpers.tcl
read_liberty sky130/sky130_tt.lib
read_lef sky130/sky130_tech.lef
read_lef sky130/sky130_std_cell.lef
read_def resize4.def

create_clock -period 1 clk
set_wire_rc -layer met1
estimate_parasitics -placement
# value carefully chosen so resize chooses dly cell over vanilla buffer
set_load .012 u1z

report_checks -fields {slew input_pin} -digits 3
resize
report_checks -fields {slew input_pin} -digits 3

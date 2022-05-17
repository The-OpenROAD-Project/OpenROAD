# resize
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def resize4.def

create_clock -period 1 clk
# value carefully chosen so resize chooses dly cell over vanilla buffer
set_load .014 u1z

source sky130hs/sky130hs.rc
set_wire_rc -layer met2
estimate_parasitics -placement

report_checks -fields {slew input_pin} -digits 3
rsz::resize_to_target_slew [get_pin "u1/X"]
report_checks -fields {slew input_pin} -digits 3

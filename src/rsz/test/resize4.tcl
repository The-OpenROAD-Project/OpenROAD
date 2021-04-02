# resize
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def resize4.def

create_clock -period 1 clk
set_wire_rc -layer met1
estimate_parasitics -placement
# value carefully chosen so resize chooses dly cell over vanilla buffer
set_load .014 u1z

report_checks -fields {slew input_pin} -digits 3
rsz::resizer_preamble
rsz::resize_driver_to_target_slew [get_pin "u1/X"]
report_checks -fields {slew input_pin} -digits 3

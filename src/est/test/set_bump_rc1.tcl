# set_bump_rc command; no bumps in the design so estimation is unchanged
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# negative values are rejected
catch { set_bump_rc -resistance -1 } msg
puts $msg

set_bump_rc -resistance 0.01 -capacitance 1.0

# same wire RC values as make_parasitics1
set lambda .12
# kohm/square.
set m1_res_sq .08e-3
# ff/micron^2
set m1_area_cap 39e-3
# ff/micron.
set m1_edge_cap 57e-3
# 4 lambda wide wire
set wire_cap [expr { $m1_area_cap * $lambda * 4 + $m1_edge_cap * 2 }]
set wire_res [expr { $m1_res_sq / ($lambda * 4) }]
set_wire_rc -resistance $wire_res -capacitance $wire_cap
estimate_parasitics -placement

report_checks

# estimate_parasitics with tech-targeted wire RC in a multi-tech database
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# a second tech makes the database multi-tech; tech-targeted values still work
read_lef -tech -tech_name tech2 Nangate45/Nangate45.lef

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
set_wire_rc -tech Nangate45 -resistance $wire_res -capacitance $wire_cap
estimate_parasitics -placement

report_checks

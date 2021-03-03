# estimate_parasitics
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# microns
set lambda .12
# kohm/square.
set m1_res_sq .08e-3
# ff/micron^2
set m1_area_cap 39e-3
# ff/micron.
set m1_edge_cap 57e-3
# 4 lambda wide wire
# ff/micron of wire length
set wire_cap [expr $m1_area_cap * $lambda * 4 + $m1_edge_cap * 2]
# kohm/micron of wire length
set wire_res [expr $m1_res_sq / ($lambda * 4)]
set_wire_rc -resistance $wire_res -capacitance $wire_cap
estimate_parasitics -placement

report_checks

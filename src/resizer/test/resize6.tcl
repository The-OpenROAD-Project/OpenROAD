# resize reg1 rebuffer/resize design area
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def rebuffer2.def
create_clock clk -period 1

set buffer_cell [get_lib_cell BUF_X2]
# kohm/micron, ff/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 13

report_design_area

repair_max_cap -buffer_cell $buffer_cell
repair_max_slew -buffer_cell $buffer_cell
resize
report_design_area

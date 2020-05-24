# rebuffer reg -> reg and output port (no insertion)
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def rebuffer7.def
create_clock clk -period 1

set buffer_cell [get_lib_cell BUF_X2]
# kohm/micron, ff/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 13
sta::rebuffer_net [get_net out] $buffer_cell

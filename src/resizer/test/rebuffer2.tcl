# rebuffer fanout 4 regs in a line driven from the middle
source "helpers.tcl"
read_lef Nangate.lef
read_liberty Nangate_typ.lib
read_def rebuffer2.def
create_clock clk -period 1

set buffer_cell [get_lib_cell BUF_X2]
# kohm/micron, ff/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 13

report_checks -fields {input_pin capacitance}

#      s5  s6  s7
#  r3  r2  r1  r4  r5
# 1,1 1,2 1,3 1,4 1,5
sta::rebuffer_net [get_net r1q] $buffer_cell
report_checks -fields {input_pin capacitance}

set def_file [make_result_file rebuffer2.def]
write_def $def_file
diff_files rebuffer2.defok $def_file

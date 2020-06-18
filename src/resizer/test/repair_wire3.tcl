# repair_long_wires auto length
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire2.def

set_wire_rc -layer metal3
# zero estimated parasitics to output ports
sta::set_pi_model u3/Z 0 0 0
sta::set_elmore u3/Z out1 0
sta::set_pi_model u4/Z 0 0 0
sta::set_elmore u4/Z out1 0

set buffer_cell [get_lib_cell BUF_X1]
report_long_wires 4
repair_long_wires -buffer_cell $buffer_cell
report_long_wires 6

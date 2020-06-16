# repair_long_wires 1 wire
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

proc set_inst_loc { inst_name x y } {
  set inst [get_cell $inst_name]
  [sta::sta_to_db_inst $inst] setOrigin [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
}

proc set_port_loc { port_name x y } {
  set port [get_port $port_name]
  set bpins [[sta::sta_to_db_port $port] getBPins]
  # don't see any way to mve the bpin
}

# in microns
set wire_length 1500
set_inst_loc u3 $wire_length 0
set_port_loc out1 $wire_length 0

# 14nm
#set_wire_rc -resistance .025 -capacitance .20
# nangate layer3 equiv
#set_wire_rc -resistance .0035 -capacitance .052
set_wire_rc -layer metal3

# zero estimated parasitics to output port
set_load 0 out1
sta::set_pi_model u3/Z 0 0 0
sta::set_elmore u3/Z out1 0

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1

report_long_wires 2

set buffer_cell [get_lib_cell BUF_X1]
set max_wire_length [sta::find_max_wire_length $buffer_cell]
puts "max wire length [sta::format_distance $max_wire_length 0]"

set max_slew 200e-12
set max_slew_wire_length [sta::find_max_slew_wire_length $max_slew $buffer_cell]
puts "max slew wire length [sta::format_distance $max_slew_wire_length 0]"

repair_long_wires -max_length 1000 -buffer_cell $buffer_cell

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1

proc set_inst_loc { inst_name x y } {
  set inst [get_cell $inst_name]
  [sta::sta_to_db_inst $inst] setOrigin [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
}

# does not work
proc set_port_loc { port_name x y } {
  set port [get_port $port_name]
  set bpins [[sta::sta_to_db_port $port] getBPins]
  # don't see any way to move the bpin
}

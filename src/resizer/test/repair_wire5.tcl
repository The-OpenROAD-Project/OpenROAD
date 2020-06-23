# repair_long_wires wire to pad
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x32.lib
read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x32.lef
read_lef pad.lef
read_def repair_wire5.def

set_wire_rc -layer metal3
estimate_parasitics -placement
repair_design -max_wire_length 600 -buffer_cell BUF_X1

# Make sure instances are inside the core.
set core [ord::get_db_core]
foreach inst [get_cells *] {
  lassign [[sta::sta_to_db_inst $inst] getLocation] x y
  if { ![$core intersects [odb::new_Point $x $y]] } {
    puts "[get_full_name $inst] outside of core"
  }
}

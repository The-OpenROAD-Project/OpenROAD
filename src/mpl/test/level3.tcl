source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

# place_pins result is not stable across ports. sigh.
if {0} {
  read_verilog level3.v
  link_design gcd_mem3

  source Nangate45/Nangate45.vars

  # macro_place fails with smaller cores
  set die_area {0 0 200 250}
  set core_area {5 5 195 245}
  
  initialize_floorplan -site $site \
    -die_area $die_area \
    -core_area $core_area
  source $tracks_file
  place_pins -random -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer
  global_placement
  
  write_def level3.def
} else {
  read_def level3.def
}
read_sdc gcd.sdc

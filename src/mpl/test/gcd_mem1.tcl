read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

# place_pins results are not portable
if {1} {
  read_verilog gcd_mem1.v
  link_design gcd_mem1

  source Nangate45/Nangate45.vars
  set die_area {0 0 120 80}
  set core_area {5 5 115 75}

  initialize_floorplan -site $site \
    -die_area $die_area \
    -core_area $core_area
  source $tracks_file

  place_pins -random -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

  global_placement
  write_def gcd_mem1.def
} else {
  read_def gcd_mem1.def
}

read_sdc gcd.sdc

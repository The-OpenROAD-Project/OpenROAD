source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

# place_pins result is not stable across ports. sigh.
if {0} {
  read_verilog east_west1.v
  link_design mem2
  source Nangate45/Nangate45.vars

  set core_width 200
  set core_height 100
  set margin 10
  set die_area "0 0 [expr $core_width + $margin * 2] [expr $core_height + $margin * 2]"
  set core_area "$margin $margin $core_width $core_height"

  initialize_floorplan -site $site \
    -die_area $die_area \
    -core_area $core_area
  source $tracks_file
  
  # place mem_out0 pins on west/east edge
  # place mem_out1 pins on east/west edge
  if { $mem0_pins_west } {
    set mem0_pin_x $margin
    set mem1_pin_x [expr $margin + $core_width]
  } else {
    set mem0_pin_x [expr $margin + $core_width]
    set mem1_pin_x $margin
  }
  set bus_width 7
  for {set i 0} {$i < $bus_width} {incr i} {
    place_pin -pin_name mem_out0[$i] -layer $io_placer_ver_layer \
      -location "$mem0_pin_x [expr $margin + $i * $core_height / $bus_width]" -pin_size {1 1}
    place_pin -pin_name mem_out1[$i] -layer $io_placer_ver_layer \
      -location "$mem1_pin_x [expr $margin + $i * $core_height / $bus_width]" -pin_size {1 1}
  }
  place_pin -pin_name clk -layer $io_placer_hor_layer \
    -location "[expr $margin + $core_width / 2] $margin" -pin_size {1 1}
  global_placement
  
  write_def east_west1.def
} else {
  read_def east_west1.def
}

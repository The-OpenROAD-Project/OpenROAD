# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024-2025, The OpenROAD Authors

sta::define_cmd_args "generate_ram_netlist" {-bytes_per_word bits
                                             -word_count words
                                             [-storage_cell name]
                                             [-tristate_cell name]
                                             [-inv_cell name]
                                             [-read_ports count]
                                             [-tapcell name]
                                             [-max_tap_dist value]}

proc generate_ram_netlist { args } {
  sta::parse_key_args "generate_ram_netlist" args \
    keys { -bytes_per_word -word_count -storage_cell -tristate_cell -inv_cell
      -read_ports -tapcell -max_tap_dist } flags {}

  if { [info exists keys(-bytes_per_word)] } {
    set bytes_per_word $keys(-bytes_per_word)
  } else {
    utl::error RAM 1 "The -bytes_per_word argument must be specified."
  }

  if { [info exists keys(-word_count)] } {
    set word_count $keys(-word_count)
  } else {
    utl::error RAM 2 "The -word_count argument must be specified."
  }

  set storage_cell ""
  if { [info exists keys(-storage_cell)] } {
    set storage_cell $keys(-storage_cell)
  }

  set tristate_cell ""
  if { [info exists keys(-tristate_cell)] } {
    set tristate_cell $keys(-tristate_cell)
  }

  set inv_cell ""
  if { [info exists keys(-inv_cell)] } {
    set inv_cell $keys(-inv_cell)
  }

  set read_ports 1
  if { [info exists keys(-read_ports)] } {
    set read_ports $keys(-read_ports)
  }

  set tapcell ""
  set max_tap_dist 0
  if { [info exists keys(-tapcell)] } {
    if { [info exists keys(-max_tap_dist)] } {
      set max_tap_dist $keys(-max_tap_dist)
      set max_tap_dist [ord::microns_to_dbu $max_tap_dist]
    } else {
      utl::error RAM 21 "The -max_tap_dist argument must be specified with tapcell."
    }
    set tapcell $keys(-tapcell)
  } else {
    utl::warn RAM 22 "No tapcell is specified.
        The generated layout may not pass Design Rule Checks."
  }

  ram::generate_ram_netlist_cmd $bytes_per_word $word_count $storage_cell \
    $tristate_cell $inv_cell $read_ports $tapcell $max_tap_dist
}

sta::define_cmd_args "generate_ram" {-bytes_per_word bits
                                     -word_count words
                                     [-read_ports count]
                                     [-storage_cell name]
                                     [-tristate_cell name]
                                     [-inv_cell name]
                                     -power_pin name
                                     -ground_pin name
                                     -routing_layer config
                                     -ver_layer config
                                     -hor_layer config
                                     -filler_cells fillers
                                     [-tapcell name]
                                     [-max_tap_dist value]}

# user arguments for generate ram arguments
proc generate_ram { args } {
  sta::parse_key_args "generate_ram" args \
    keys { -bytes_per_word -word_count -storage_cell -tristate_cell -inv_cell -read_ports
      -power_pin -ground_pin -routing_layer -ver_layer -hor_layer -filler_cells
        -tapcell -max_tap_dist } flags {}

  sta::check_argc_eq0 "generate_ram" $args

  # Check for valid design
  if { [ord::get_db_block] != "NULL" } {
    utl::error RAM 20 "A design is already loaded. Cannot generate RAM"
  }

  set ram_netlist_args [list \
    -bytes_per_word $keys(-bytes_per_word) \
    -word_count $keys(-word_count)]

  if { [info exists keys(-read_ports)] } {
    lappend ram_netlist_args -read_ports $keys(-read_ports)
  }

  if { [info exists keys(-storage_cell)] } {
    lappend ram_netlist_args -storage_cell $keys(-storage_cell)
  }

  if { [info exists keys(-tristate_cell)] } {
    lappend ram_netlist_args -tristate_cell $keys(-tristate_cell)
  }

  if { [info exists keys(-inv_cell)] } {
    lappend ram_netlist_args -inv_cell $keys(-inv_cell)
  }

  if { [info exists keys(-tapcell)] } {
    lappend ram_netlist_args -tapcell $keys(-tapcell)
  }

  if { [info exists keys(-max_tap_dist)] } {
    lappend ram_netlist_args -max_tap_dist $keys(-max_tap_dist)
  }

  generate_ram_netlist {*}$ram_netlist_args

  ord::design_created

  if { [info exists keys(-power_pin)] } {
    set power_pin $keys(-power_pin)
  } else {
    utl::error RAM 5 "The -power_pin argument must be specified."
  }

  if { [info exists keys(-ground_pin)] } {
    set ground_pin $keys(-ground_pin)
  } else {
    utl::error RAM 6 "The -ground_pin argument must be specified."
  }

  if { [info exists keys(-routing_layer)] } {
    set routing_layer $keys(-routing_layer)
  } else {
    utl::error RAM 9 "The -routing_layer argument must be specified."
  }

  if { [llength $routing_layer] != 2 } {
    utl::error RAM 12 "-routing_layer is not a list of 2 values"
  } else {
    lassign $routing_layer route_name route_width
    set route_width [ord::microns_to_dbu $route_width]
  }

  if { [info exists keys(-ver_layer)] } {
    set ver_layer $keys(-ver_layer)
  } else {
    utl::error RAM 13 "The -ver_layer argument must be specified."
  }

  if { [llength $ver_layer] != 3 } {
    utl::error RAM 14 "-ver_layer is not a list of 2 values"
  } else {
    lassign $ver_layer ver_name ver_width ver_pitch
    set ver_width [ord::microns_to_dbu $ver_width]
    set ver_pitch [ord::microns_to_dbu $ver_pitch]
  }

  if { [info exists keys(-hor_layer)] } {
    set hor_layer $keys(-hor_layer)
  } else {
    utl::error RAM 15 "The -hor_layer argument must be specified."
  }

  if { [llength $hor_layer] != 3 } {
    utl::error RAM 17 "-hor_layer is not a list of 2 values"
  } else {
    lassign $hor_layer hor_name hor_width hor_pitch
    set hor_width [ord::microns_to_dbu $hor_width]
    set hor_pitch [ord::microns_to_dbu $hor_pitch]
  }

  if { [info exists keys(-filler_cells)] } {
    set filler_cells $keys(-filler_cells)
  } else {
    utl::error RAM 18 "The -filler_cells argument must be specified."
  }

  ram::ram_pdngen $power_pin $ground_pin $route_name $route_width \
    $ver_name $ver_width $ver_pitch $hor_name $hor_width $hor_pitch

  make_tracks -x_offset 0 -y_offset 0

  ram::ram_pinplacer $ver_name $hor_name

  ram::ram_filler $filler_cells

  ram::ram_routing
}

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024-2025, The OpenROAD Authors

sta::define_cmd_args "generate_ram_netlist" {[-mask_size bits]
                                             -word_size bits
                                             -num_words words
                                             [-column_mux_ratio ratio]
                                             [-storage_cell name]
                                             [-tristate_cell name]
                                             [-read_style tristate|and_or]
                                             [-zero_word word]
                                             [-reset_value value]
                                             [-banks count]
                                             [-bank_columns count]
                                             [-bank_order contiguous|interleaved]
                                             [-rb_ports count]
                                             [-read_latency 0|1]
                                             [-write_style clock_gate|mux]
                                             [-cells {role master ...}]
                                             [-name name]
                                             [-decoder row|word]
                                             [-periphery placed|unplaced]
                                             [-pin_layer layer]
                                             [-inv_cell name]
                                             [-rw_ports count]
                                             [-r_ports count]
                                             [-w_ports count]
                                             [-use_latch value]
                                             [-tapcell name]
                                             [-max_tap_dist value]}

proc generate_ram_netlist { args } {
  sta::parse_key_args "generate_ram_netlist" args \
    keys { -mask_size -word_size -num_words -column_mux_ratio -storage_cell -tristate_cell -inv_cell
      -rw_ports -r_ports -w_ports -use_latch -tapcell -max_tap_dist
      -write_behavioral_verilog -read_style -zero_word -reset_value
      -banks -bank_columns -bank_order -rb_ports -read_latency -write_style
      -periphery -pin_layer -cells -name -decoder } flags {}

  set column_mux_ratio 1
  if { [info exists keys(-column_mux_ratio)] } {
    set column_mux_ratio $keys(-column_mux_ratio)
  }

  if { [info exists keys(-word_size)] } {
    set word_size $keys(-word_size)
  } else {
    utl::error RAM 2 "The -word_size argument must be specified."
  }

  if { [info exists keys(-mask_size)] } {
    set mask_size $keys(-mask_size)
  } else {
    set mask_size $word_size
  }

  if { $word_size % $mask_size != 0 } {
    utl::error RAM 26 "The -word_size ($word_size) must be divisible by -mask_size ($mask_size)."
  }

  if { [info exists keys(-num_words)] } {
    set num_words $keys(-num_words)
  } else {
    utl::error RAM 27 "The -num_words argument must be specified."
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

  set rw_ports 0
  if { [info exists keys(-rw_ports)] } {
    set rw_ports $keys(-rw_ports)
  }

  set r_ports 0
  if { [info exists keys(-r_ports)] } {
    set r_ports $keys(-r_ports)
  }

  set w_ports 0
  if { [info exists keys(-w_ports)] } {
    set w_ports $keys(-w_ports)
  }

  # Defaults to single read/write port if no port flags were specified
  if {
    ![info exists keys(-rw_ports)] &&
    ![info exists keys(-r_ports)] &&
    ![info exists keys(-w_ports)]
  } {
    set rw_ports 1
  }

  if { $rw_ports + $w_ports < 1 } {
    utl::error RAM 30 "Must have a read/write port or a write port."
  }

  if { $rw_ports > 1 || ($rw_ports > 0 && $w_ports > 0) } {
    utl::error RAM 53 "A read/write port is the only write port."
  }

  if { $rw_ports + $r_ports < 1 && ![info exists keys(-rb_ports)] } {
    utl::error RAM 36 "Must specify at least one read output port"
  }

  set use_latch 0
  if { [info exists keys(-use_latch)] } {
    set use_latch $keys(-use_latch)
  }

  # More than one write port: each has its own address decoder and its
  # own data, a word written by two in a cycle keeps the last one's.
  if { $w_ports > 1 && $use_latch } {
    utl::error RAM 54 "More than one write port is not supported with -use_latch."
  }
  if { $w_ports > 1 && $column_mux_ratio > 1 } {
    utl::error RAM 55 "More than one write port is not supported with\
      -column_mux_ratio above 1."
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

  if { [info exists keys(-write_behavioral_verilog)] } {
    ram::set_behavioral_verilog_filename $keys(-write_behavioral_verilog)
  }

  # The read path: tristate bitlines, or AND-OR trees (and_or). By default
  # tristate where the library has a tristate cell, and_or where not.
  set read_style ""
  if { [info exists keys(-read_style)] } {
    set read_style $keys(-read_style)
    if { $read_style ni {tristate and_or} } {
      utl::error RAM 56 "-read_style is tristate or and_or, not $read_style."
    }
  }

  # A word that reads 0 and drops its writes, as a RISC-V x0.
  set zero_word -1
  if { [info exists keys(-zero_word)] } {
    set zero_word $keys(-zero_word)
  }

  # An asynchronous reset of every word to this value.
  set reset_value ""
  if { [info exists keys(-reset_value)] } {
    set reset_value $keys(-reset_value)
  }

  # Banks: the words in banks, contiguous or interleaved addresses,
  # bank_columns of them side by side and the rest stacked.
  set banks 1
  if { [info exists keys(-banks)] } {
    set banks $keys(-banks)
  }
  set bank_columns 1
  if { [info exists keys(-bank_columns)] } {
    set bank_columns $keys(-bank_columns)
  }
  # The periphery placed with the array (placed), or left for a parent to
  # place, the outline then the core alone and the pins on -pin_layer
  # where their connections land (unplaced).
  set periphery placed
  if { [info exists keys(-periphery)] } {
    set periphery $keys(-periphery)
    if { $periphery ni {placed unplaced} } {
      utl::error RAM 77 "-periphery is placed or unplaced, not $periphery."
    }
  }
  set pin_layer ""
  if { [info exists keys(-pin_layer)] } {
    set pin_layer $keys(-pin_layer)
  }

  # {role master ...}: the cell for a role, instead of the smallest of its
  # function; and the block's name, instead of RAM<words>x<bits>
  set cells ""
  if { [info exists keys(-cells)] } {
    set cells [join $keys(-cells) " "]
  }
  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }

  # The address decoders: row, a decoder per row (an AND2 chain), the
  # column mux selecting within it; or word, a balanced AND2 tree over
  # each word's whole address.
  set decoder row
  if { [info exists keys(-decoder)] } {
    set decoder $keys(-decoder)
    if { $decoder ni {row word} } {
      utl::error RAM 82 "-decoder is row or word, not $decoder."
    }
  }

  # How a word holds its value between writes: a clock gate per word
  # (clock_gate), or a mux per bit (mux).
  set write_style clock_gate
  if { [info exists keys(-write_style)] } {
    set write_style $keys(-write_style)
    if { $write_style ni {clock_gate mux} } {
      utl::error RAM 74 "-write_style is clock_gate or mux, not $write_style."
    }
  }

  # A read latency of 1: the read addresses registered.
  set read_latency 0
  if { [info exists keys(-read_latency)] } {
    set read_latency $keys(-read_latency)
  }

  # Banked read ports: an address and a data output per bank.
  set rb_ports 0
  if { [info exists keys(-rb_ports)] } {
    set rb_ports $keys(-rb_ports)
  }
  set bank_order contiguous
  if { [info exists keys(-bank_order)] } {
    set bank_order $keys(-bank_order)
    if { $bank_order ni {contiguous interleaved} } {
      utl::error RAM 69 "-bank_order is contiguous or interleaved, not $bank_order."
    }
  }

  ram::generate_ram_netlist_cmd $mask_size $word_size $num_words $rw_ports $r_ports $w_ports \
    $column_mux_ratio $use_latch $storage_cell $tristate_cell $inv_cell $tapcell $max_tap_dist \
    $read_style $zero_word $reset_value $banks $bank_columns $bank_order \
    $rb_ports $read_latency $write_style $periphery $pin_layer $cells $name \
    $decoder
}

sta::define_cmd_args "generate_ram" {[-mask_size bits]
                                     -word_size bits
                                     -num_words words
                                     [-rw_ports count]
                                     [-r_ports count]
                                     [-w_ports count]
                                     [-column_mux_ratio ratio]
                                     [-storage_cell name]
                                     [-tristate_cell name]
                                     [-read_style tristate|and_or]
                                     [-inv_cell name]
                                     [-power_net_name name]
                                     [-ground_net_name name]
                                     -routing_layer config
                                     -ver_layer config
                                     -hor_layer config
                                     -filler_cells fillers
                                     [-tapcell name]
                                     [-max_tap_dist value]
                                     [-use_latch value]
                                     [-write_behavioral_verilog filename]
                                     }

# user arguments for generate ram arguments
proc generate_ram { args } {
  sta::parse_key_args "generate_ram" args \
    keys { -mask_size -word_size -num_words -column_mux_ratio
           -storage_cell -tristate_cell -inv_cell -rw_ports
           -r_ports -w_ports -use_latch -power_net_name
           -ground_net_name -routing_layer -ver_layer
           -hor_layer -filler_cells -tapcell -max_tap_dist
           -write_behavioral_verilog -read_style -zero_word -reset_value
           -banks -bank_columns -bank_order -rb_ports -read_latency -write_style
           -cells -name -decoder } flags {}

  sta::check_argc_eq0 "generate_ram" $args

  # Check for valid design
  if { [ord::get_db_block] != "NULL" } {
    utl::error RAM 20 "A design is already loaded. Cannot generate RAM"
  }

  set ram_netlist_args [list \
    -word_size $keys(-word_size) \
    -num_words $keys(-num_words)]

  if { [info exists keys(-rw_ports)] } {
    lappend ram_netlist_args -rw_ports $keys(-rw_ports)
  }

  if { [info exists keys(-r_ports)] } {
    lappend ram_netlist_args -r_ports $keys(-r_ports)
  }

  if { [info exists keys(-w_ports)] } {
    lappend ram_netlist_args -w_ports $keys(-w_ports)
  }

  if { [info exists keys(-mask_size)] } {
    lappend ram_netlist_args -mask_size $keys(-mask_size)
  }

  if { [info exists keys(-storage_cell)] } {
    lappend ram_netlist_args -storage_cell $keys(-storage_cell)
  }

  if { [info exists keys(-tristate_cell)] } {
    lappend ram_netlist_args -tristate_cell $keys(-tristate_cell)
  }

  if { [info exists keys(-read_style)] } {
    lappend ram_netlist_args -read_style $keys(-read_style)
  }

  if { [info exists keys(-zero_word)] } {
    lappend ram_netlist_args -zero_word $keys(-zero_word)
  }

  if { [info exists keys(-reset_value)] } {
    lappend ram_netlist_args -reset_value $keys(-reset_value)
  }

  foreach key {-banks -bank_columns -bank_order -rb_ports -read_latency -write_style
               -cells -name -decoder} {
    if { [info exists keys($key)] } {
      lappend ram_netlist_args $key $keys($key)
    }
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

  if { [info exists keys(-column_mux_ratio)] } {
    lappend ram_netlist_args -column_mux_ratio $keys(-column_mux_ratio)
  }

  if { [info exists keys(-write_behavioral_verilog)] } {
    set behavioral_verilog_file $keys(-write_behavioral_verilog)
    ram::set_behavioral_verilog_filename $behavioral_verilog_file
  }

  if { [info exists keys(-use_latch)] } {
    lappend ram_netlist_args -use_latch $keys(-use_latch)
  }

  generate_ram_netlist {*}$ram_netlist_args

  ord::design_created

  set power_net_name "VDD"
  if { [info exists keys(-power_net_name)] } {
    set power_net_name $keys(-power_net_name)
  }

  set power_net_name [string trim $power_net_name]
  if { $power_net_name eq "" } {
    utl::error RAM 40 "The -power_net_name argument cannot be empty."
  }

  set ground_net_name "VSS"
  if { [info exists keys(-ground_net_name)] } {
    set ground_net_name $keys(-ground_net_name)
  }

  set ground_net_name [string trim $ground_net_name]
  if { $ground_net_name eq "" } {
    utl::error RAM 41 "The -ground_net_name argument cannot be empty."
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

  ram::ram_pdngen $power_net_name $ground_net_name \
    $route_name $route_width \
    $ver_name $ver_width $ver_pitch $hor_name $hor_width $hor_pitch

  make_tracks -x_offset 0 -y_offset 0

  ram::ram_pinplacer $ver_name $hor_name

  ram::ram_filler $filler_cells

  ram::ram_routing
}

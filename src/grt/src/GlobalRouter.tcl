# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

sta::define_cmd_args "set_global_routing_layer_adjustment" { layer adj }

proc set_global_routing_layer_adjustment { args } {
  sta::parse_key_args "set_global_routing_layer_adjustment" args \
    keys {} \
    flags {}

  if { [llength $args] == 2 } {
    lassign $args layer adj

    if { $layer == "*" } {
      sta::check_float "adjustment" $adj
      grt::set_capacity_adjustment $adj
    } elseif { [regexp -all {([^-]+)-([^ ]+)} $layer] } {
      lassign [grt::parse_layer_range "set_global_routing_layer_adjustment" \
        $layer] first_layer last_layer
      for { set l $first_layer } { $l <= $last_layer } { incr l } {
        grt::check_routing_layer $l
        sta::check_float "adjustment" $adj

        grt::add_layer_adjustment $l $adj
      }
    } else {
      set layer_idx [grt::parse_layer_name $layer]
      grt::check_routing_layer $layer_idx
      sta::check_float "adjustment" $adj

      grt::add_layer_adjustment $layer_idx $adj
    }
  } else {
    utl::error GRT 44 "set_global_routing_layer_adjustment requires layer and adj arguments."
  }
}

sta::define_cmd_args "set_global_routing_region_adjustment" { region \
                                                              [-layer layer] \
                                                              [-adjustment adjustment] \
}

proc set_global_routing_region_adjustment { args } {
  sta::parse_key_args "set_global_routing_region_adjustment" args \
    keys {-layer -adjustment} flags {}

  if { ![ord::db_has_tech] } {
    utl::error GRT 47 "Missing dbTech."
  }
  set tech [ord::get_db_tech]
  set lef_units [$tech getDbUnitsPerMicron]

  if { [info exists keys(-layer)] } {
    set layer $keys(-layer)
  } else {
    utl::error GRT 48 "Command set_global_routing_region_adjustment is missing -layer argument."
  }

  if { [info exists keys(-adjustment)] } {
    set adjustment $keys(-adjustment)
  } else {
    utl::error GRT 49 \
      "Command set_global_routing_region_adjustment is missing -adjustment argument."
  }

  sta::check_argc_eq1 "set_global_routing_region_adjustment" $args
  set region [lindex $args 0]
  if { [llength $region] == 4 } {
    lassign $region lower_x lower_y upper_x upper_y
    sta::check_positive_float "lower_left_x" $lower_x
    sta::check_positive_float "lower_left_y" $lower_y
    sta::check_positive_float "upper_right_x" $upper_x
    sta::check_positive_float "upper_right_y" $upper_y
    sta::check_positive_float "-adjustment" $adjustment

    set layer_idx [grt::parse_layer_name $layer]
    grt::check_routing_layer $layer_idx

    set lower_x [expr { int($lower_x * $lef_units) }]
    set lower_y [expr { int($lower_y * $lef_units) }]
    set upper_x [expr { int($upper_x * $lef_units) }]
    set upper_y [expr { int($upper_y * $lef_units) }]

    grt::check_region $lower_x $lower_y $upper_x $upper_y

    grt::add_region_adjustment $lower_x $lower_y $upper_x $upper_y $layer_idx $adjustment
  } else {
    utl::error GRT 50 \
      "Command set_global_routing_region_adjustment needs four arguments\
       to define a region: lower_x lower_y upper_x upper_y."
  }
}

sta::define_cmd_args "set_macro_extension" { extension }

proc set_macro_extension { args } {
  sta::parse_key_args "set_macro_extension" args \
    keys {} \
    flags {}
  if { [llength $args] == 1 } {
    lassign $args extension
    sta::check_positive_integer "macro_extension" $extension
    grt::set_macro_extension $extension
  } else {
    utl::error GRT 219 "Command set_macro_extension needs one argument: extension."
  }
}

sta::define_cmd_args "set_global_routing_random" { [-seed seed] \
                                                   [-capacities_perturbation_percentage percent] \
                                                   [-perturbation_amount value]
                                                 }

proc set_global_routing_random { args } {
  sta::parse_key_args "set_global_routing_random" args \
    keys { -seed -capacities_perturbation_percentage -perturbation_amount } flags {}

  sta::check_argc_eq0 "set_global_routing_random" $args

  if { [info exists keys(-seed)] } {
    set seed $keys(-seed)
    sta::check_integer "set_global_routing_random" $seed
    grt::set_seed $seed
  } else {
    utl::error GRT 242 "-seed argument is required."
  }

  set percentage 0.0
  if { [info exists keys(-capacities_perturbation_percentage)] } {
    set percentage $keys(-capacities_perturbation_percentage)
    sta::check_percent "set_global_routing_random" $percentage
  }
  grt::set_capacities_perturbation_percentage $percentage

  set perturbation 1
  if { [info exists keys(-perturbation_amount)] } {
    set perturbation $keys(-perturbation_amount)
    sta::check_positive_integer "set_global_routing_random" $perturbation
  }
  grt::set_perturbation_amount $perturbation
}

sta::define_cmd_args "global_route" {[-guide_file out_file] \
                                  [-congestion_iterations iterations] \
                                  [-congestion_report_file file_name] \
                                  [-congestion_report_iter_step steps] \
                                  [-grid_origin origin] \
                                  [-critical_nets_percentage percent] \
                                  [-skip_large_fanout_nets fanout] \
                                  [-allow_congestion] \
                                  [-verbose] \
                                  [-start_incremental] \
                                  [-end_incremental] \
                                  [-use_cugr] \
                                  [-resistance_aware]
}

proc global_route { args } {
  sta::parse_key_args "global_route" args \
    keys {-guide_file -congestion_iterations -congestion_report_file \
          -grid_origin -critical_nets_percentage -congestion_report_iter_step\
          -skip_large_fanout_nets
         } \
    flags {-allow_congestion -resistance_aware -verbose -start_incremental -end_incremental \
          -use_cugr}

  sta::check_argc_eq0 "global_route" $args

  if { ![ord::db_has_tech] } {
    utl::error GRT 51 "Missing dbTech."
  }

  if { [ord::get_db_block] == "NULL" } {
    utl::error GRT 52 "Missing dbBlock."
  }

  grt::set_use_cugr [info exists flags(-use_cugr)]

  grt::set_verbose [info exists flags(-verbose)]

  if { [info exists keys(-grid_origin)] } {
    set origin $keys(-grid_origin)
    if { [llength $origin] == 2 } {
      lassign $origin origin_x origin_y
      grt::set_grid_origin $origin_x $origin_y
    } else {
      utl::error GRT 55 "Wrong number of arguments for origin."
    }
  } else {
    grt::set_grid_origin 0 0
  }

  if { [info exists keys(-congestion_iterations)] } {
    set iterations $keys(-congestion_iterations)
    sta::check_positive_integer "-congestion_iterations" $iterations
    grt::set_congestion_iterations $iterations
  } else {
    grt::set_congestion_iterations 50
  }

  if { [info exists keys(-congestion_report_file)] } {
    set file_name $keys(-congestion_report_file)
    grt::set_congestion_report_file $file_name
  }

  if { [info exists keys(-congestion_report_iter_step)] } {
    set steps $keys(-congestion_report_iter_step)
    grt::set_congestion_report_iter_step $steps
  } else {
    grt::set_congestion_report_iter_step 0
  }

  if { [info exists keys(-critical_nets_percentage)] } {
    set percentage $keys(-critical_nets_percentage)
    sta::check_percent "-critical_nets_percentage" $percentage
    grt::set_critical_nets_percentage $percentage
  }

  if { [info exists keys(-skip_large_fanout_nets)] } {
    set fanout $keys(-skip_large_fanout_nets)
    sta::check_positive_integer "-skip_large_fanout_nets" $fanout
    grt::set_skip_large_fanout $fanout
  }

  set allow_congestion [info exists flags(-allow_congestion)]
  grt::set_allow_congestion $allow_congestion

  set resistance_aware [info exists flags(-resistance_aware)]
  grt::set_resistance_aware $resistance_aware

  set start_incremental [info exists flags(-start_incremental)]
  set end_incremental [info exists flags(-end_incremental)]

  if { $start_incremental && $end_incremental } {
    utl::error GRT 295 "Only one of -start_incremental or -end_incremental can be used."
  } elseif { $start_incremental } {
    grt::start_incremental
  } elseif { $end_incremental } {
    grt::end_incremental
  } else {
    grt::global_route
  }

  if { [info exists keys(-guide_file)] } {
    set out_file $keys(-guide_file)
    write_guides $out_file
  }
}

sta::define_cmd_args "repair_antennas" { diode_cell \
                                         [-iterations iterations] \
                                         [-ratio_margin ratio_margin] \
                                         [-jumper_only] \
                                         [-diode_only] \
                                         [-allow_congestion]}

proc repair_antennas { args } {
  sta::parse_key_args "repair_antennas" args \
    keys {-iterations -ratio_margin} flags {-jumper_only -diode_only -allow_congestion}
  if { [ord::get_db_block] == "NULL" } {
    utl::error GRT 104 "No design block found."
  }

  if { [grt::have_routes] || [grt::have_detailed_routes] } {
    if { [llength $args] == 0 } {
      # repairAntennas locates diode
      set diode_mterm "NULL"
    } elseif { [llength $args] == 1 } {
      set db [ord::get_db]
      set diode_cell [lindex $args 0]

      set diode_master [$db findMaster $diode_cell]
      if { $diode_master == "NULL" } {
        utl::error GRT 69 "Diode cell $diode_cell not found."
      }

      set diode_mterms [$diode_master getMTerms]
      set non_pg_count 0
      foreach mterm $diode_mterms {
        if { [$mterm getSigType] != "POWER" && [$mterm getSigType] != "GROUND" } {
          set diode_mterm $mterm
          incr non_pg_count
        }
      }

      if { $non_pg_count > 1 } {
        utl::error GRT 73 "Diode cell has more than one non power/ground port."
      }
    } else {
      utl::error GRT 245 "Too arguments to repair_antennas."
    }

    set iterations 1
    if { [info exists keys(-iterations)] } {
      set iterations $keys(-iterations)
      sta::check_positive_integer "-iterations" $iterations
    }

    set allow_congestion [info exists flags(-allow_congestion)]
    grt::set_allow_congestion $allow_congestion

    set jumper_only [info exists flags(-jumper_only)]
    set diode_only [info exists flags(-diode_only)]

    if { $jumper_only && $diode_only } {
      utl::error GRT 294 "Only use either -jumper_only or -diode_only flag"
    }

    set ratio_margin 0
    if { [info exists keys(-ratio_margin)] } {
      set ratio_margin $keys(-ratio_margin)
      if { !($ratio_margin >= 0 && $ratio_margin < 100) } {
        utl::warn GRT 215 "-ratio_margin must be between 0 and 100 percent."
      }
    }

    return [grt::repair_antennas $diode_mterm $iterations $ratio_margin $jumper_only $diode_only]
  } else {
    utl::error GRT 45 "Run global_route before repair_antennas."
  }
}

sta::define_cmd_args "set_nets_to_route" { net_names }

proc set_nets_to_route { args } {
  sta::parse_key_args "set_nets_to_route" args \
    keys {} \
    flags {}
  sta::check_argc_eq1 "set_nets_to_route" $args
  set net_names [lindex $args 0]
  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 252 "Missing dbBlock."
  }

  foreach net [get_nets $net_names] {
    if { $net != "NULL" } {
      grt::add_net_to_route [sta::sta_to_db_net $net]
    }
  }
}

sta::define_cmd_args "read_guides" { file_name }

proc read_guides { args } {
  sta::parse_key_args "read_guides" args \
    keys {} \
    flags {}
  set file_name $args

  grt::read_guides $file_name
}

sta::define_cmd_args "draw_route_segments" { net_names \
                                           [-show_pin_locations] }

proc draw_route_segments { args } {
  sta::parse_key_args "draw_route_segments" args \
    keys {} \
    flags {-show_pin_locations}
  sta::check_argc_eq1 "draw_route_segments" $args
  set net_names [lindex $args 0]
  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 223 "Missing dbBlock."
  }

  grt::clear_route_guides
  set show_pins [info exists flags(-show_pin_locations)]
  foreach net [get_nets $net_names] {
    if { $net != "NULL" } {
      grt::highlight_net_route [sta::sta_to_db_net $net] $show_pins
    }
  }
}

sta::define_cmd_args "write_global_route_segments" { file_name }

proc write_global_route_segments { args } {
  sta::parse_key_args "write_global_route_segments" args \
    keys {} \
    flags {}
  set file_name $args
  grt::write_segments $file_name
}

sta::define_cmd_args "read_global_route_segments" { file_name }

proc read_global_route_segments { args } {
  sta::parse_key_args "read_global_route_segments" args \
    keys {} \
    flags {}
  set file_name $args
  grt::read_segments $file_name
}

sta::define_cmd_args "global_route_debug" {
  [-st]       # Show the Steiner Tree generated by stt
  [-rst]      # Show the Rectilinear Steiner Tree generated by FastRoute
  [-tree2D]   # Show the Rectilinear Steiner Tree generated by FastRoute after overflow iterations
  [-tree3D]   # Show The Rectilinear Steiner Tree 3D after layer assignment
  [-saveSttInput file_name] # Save the stt input for a net on file_name
  [-net name]
}

proc global_route_debug { args } {
  sta::parse_key_args "global_route_debug" args \
    keys {-saveSttInput -net} \
    flags {-st -rst -tree2D -tree3D}

  sta::check_argc_eq0 "global_route_debug" $args

  set st [info exists flags(-st)]
  set rst [info exists flags(-rst)]
  set tree2D [info exists flags(-tree2D)]
  set tree3D [info exists flags(-tree3D)]
  set db_block [ord::get_db_block]

  if { [info exists keys(-net)] } {
    set net [$db_block findNet $keys(-net)]
    if { $net == "NULL" } {
      utl::error GRT 231 "Net name not found."
    }
    grt::set_global_route_debug_cmd $net $st $rst $tree2D $tree3D
    if { [info exists keys(-saveSttInput)] } {
      set file_name $keys(-saveSttInput)
      grt::set_global_route_debug_stt_input_filename $file_name
    }
  } else {
    utl::error GRT 269 "-net argument is required."
  }
}

sta::define_cmd_args "report_wire_length" { [-net net_list] \
                                            [-file file] \
                                            [-global_route] \
                                            [-detailed_route] \
                                            [-verbose] \
                                            [-summary]
}

proc report_wire_length { args } {
  sta::parse_key_args "report_wire_length" args \
    keys {-net -file} \
    flags {-global_route -detailed_route -verbose -summary}

  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 224 "Missing dbBlock."
  }

  set global_route_wl [info exists flags(-global_route)]
  set detailed_route_wl [info exists flags(-detailed_route)]
  set verbose [info exists flags(-verbose)]

  if { !$global_route_wl && !$detailed_route_wl } {
    set global_route_wl [grt::have_routes]
    set detailed_route_wl [grt::have_detailed_route $block]
  }

  set file ""
  if { [info exists keys(-file)] } {
    set file $keys(-file)
    grt::create_wl_report_file $file $verbose
  }

  if { [info exists flags(-summary)] } {
    grt::report_layer_wire_lengths $global_route_wl $detailed_route_wl
  } elseif { [info exists keys(-net)] } {
    foreach net [get_nets $keys(-net)] {
      set db_net [sta::sta_to_db_net $net]
      if {
        [$db_net getSigType] != "POWER" &&
        [$db_net getSigType] != "GROUND" &&
        ![$db_net isSpecial]
      } {
        grt::report_net_wire_length $db_net $global_route_wl $detailed_route_wl $verbose $file
      }
    }
  } else {
    utl::error GRT 238 "-net is required."
  }
}

sta::define_cmd_args "estimate_path_resistance" { pin1_name pin2_name \
                                                  [-layer1 layer1] \
                                                  [-layer2 layer2] \
                                                  [-verbose] }

proc estimate_path_resistance { args } {
  sta::parse_key_args "estimate_path_resistance" args \
    keys {-layer1 -layer2} \
    flags {-verbose}

  if { [llength $args] != 2 } {
    utl::error GRT 289 "estimate_path_resistance requires two pin names."
  }
  lassign $args pin1_name pin2_name

  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 290 "Missing dbBlock."
  }

  set pin1 [$block findITerm $pin1_name]
  if { $pin1 != "NULL" } {
    set pin1 [grt::iterm_to_object $pin1]
  } else {
    set pin1 [$block findBTerm $pin1_name]
    if { $pin1 != "NULL" } {
      set pin1 [grt::bterm_to_object $pin1]
    }
  }
  if { $pin1 == "NULL" } {
    utl::error GRT 291 "Pin $pin1_name not found."
  }

  set pin2 [$block findITerm $pin2_name]
  if { $pin2 != "NULL" } {
    set pin2 [grt::iterm_to_object $pin2]
  } else {
    set pin2 [$block findBTerm $pin2_name]
    if { $pin2 != "NULL" } {
      set pin2 [grt::bterm_to_object $pin2]
    }
  }
  if { $pin2 == "NULL" } {
    utl::error GRT 292 "Pin $pin2_name not found."
  }

  set verbose [info exists flags(-verbose)]

  if { [info exists keys(-layer1)] && [info exists keys(-layer2)] } {
    set layer1_name $keys(-layer1)
    set layer2_name $keys(-layer2)

    set tech [ord::get_db_tech]
    if { [info exists layer1_name] } {
      set layer1 [$tech findLayer $layer1_name]
    }
    if { $layer1 == "NULL" } {
      utl::error GRT 293 "Layer $layer1_name not found."
    }

    if { [info exists layer2_name] } {
      set layer2 [$tech findLayer $layer2_name]
    }
    if { $layer2 == "NULL" } {
      utl::error GRT 287 "Layer $layer2_name not found."
    }

    return [grt::estimate_path_resistance $pin1 $pin2 $layer1 $layer2 $verbose]
  } elseif { [info exists keys(-layer1)] || [info exists keys(-layer2)] } {
    utl::error GRT 288 "Both or neither -layer1 and -layer2 must be provided."
  }

  return [grt::estimate_path_resistance $pin1 $pin2 $verbose]
}

namespace eval grt {
proc check_routing_layer { layer } {
  if { ![ord::db_has_tech] } {
    utl::error GRT 59 "Missing technology file."
  }
  sta::check_positive_integer "layer" $layer

  set tech [ord::get_db_tech]
  set max_routing_layer [$tech getRoutingLayerCount]
  set tech_layer [$tech findRoutingLayer $layer]

  set min_tech_layer [$tech findRoutingLayer 1]
  set max_tech_layer [$tech findRoutingLayer $max_routing_layer]

  if { $layer > $max_routing_layer } {
    utl::error GRT 60 "Layer [$tech_layer getConstName] is greater than\
      the max routing layer ([$max_tech_layer getConstName])."
  }
  if { $layer < 1 } {
    utl::error GRT 61 "Layer [$tech_layer getConstName] is less than the\
      min routing layer ([$min_tech_layer getConstName])."
  }
}

proc parse_layer_name { layer_name } {
  if { ![ord::db_has_tech] } {
    utl::error GRT 222 "No technology has been read."
  }
  set tech [ord::get_db_tech]
  set tech_layer [$tech findLayer $layer_name]
  if { $tech_layer == "NULL" } {
    utl::error GRT 5 "Layer $layer_name not found."
  }
  set layer_idx [$tech_layer getRoutingLevel]

  return $layer_idx
}

proc parse_layer_range { cmd layer_range } {
  if { [regexp -all {([^-]+)-([^ ]+)} $layer_range - min_layer_name max_layer_name] } {
    set min_layer [parse_layer_name $min_layer_name]
    set max_layer [parse_layer_name $max_layer_name]

    set layers "$min_layer $max_layer"
    return $layers
  } else {
    utl::error GRT 62 "Input format to define layer range for $cmd is min-max."
  }
}

proc check_region { lower_x lower_y upper_x upper_y } {
  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 63 "Missing dbBlock."
  }

  set core_area [$block getDieArea]

  if { $lower_x < [$core_area xMin] || $lower_x > [$core_area xMax] } {
    utl::error GRT 64 "Lower left x is outside die area."
  }

  if { $lower_y < [$core_area yMin] || $lower_y > [$core_area yMax] } {
    utl::error GRT 65 "Lower left y is outside die area."
  }

  if { $upper_x < [$core_area xMin] || $upper_x > [$core_area xMax] } {
    utl::error GRT 66 "Upper right x is outside die area."
  }

  if { $upper_y < [$core_area yMin] || $upper_y > [$core_area yMax] } {
    utl::error GRT 67 "Upper right y is outside die area."
  }
}

proc have_detailed_route { block } {
  set nets [$block getNets]
  foreach net $nets {
    if { [$net getWire] != "NULL" } {
      return 1
    }
  }

  return 0
}

# grt namespace end
}

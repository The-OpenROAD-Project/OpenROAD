# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

sta::define_cmd_args "set_pin_length" {[-hor_length h_length]\
                                       [-ver_length v_length]
}

proc set_pin_length { args } {
  sta::parse_key_args "set_pin_length" args \
    keys {-hor_length -ver_length} flags {}

  sta::check_argc_eq0 "set_pin_length" $args

  if { [info exists keys(-hor_length)] } {
    ppl::set_hor_length [ord::microns_to_dbu $keys(-hor_length)]
  }

  if { [info exists keys(-ver_length)] } {
    ppl::set_ver_length [ord::microns_to_dbu $keys(-ver_length)]
  }
}

sta::define_cmd_args "set_pin_length_extension" {[-hor_extension h_ext]\
                                                 [-ver_extension v_ext]
}

proc set_pin_length_extension { args } {
  sta::parse_key_args "set_pin_length_extension" args \
    keys {-hor_extension -ver_extension} flags {}

  sta::check_argc_eq0 "set_pin_length_extension" $args

  if { [info exists keys(-hor_extension)] } {
    ppl::set_hor_length_extend [ord::microns_to_dbu $keys(-hor_extension)]
  }

  if { [info exists keys(-ver_extension)] } {
    ppl::set_ver_length_extend [ord::microns_to_dbu $keys(-ver_extension)]
  }
}

sta::define_cmd_args "set_pin_thick_multiplier" {[-hor_multiplier h_mult]\
                                                 [-ver_multiplier v_mult]
}

proc set_pin_thick_multiplier { args } {
  sta::parse_key_args "set_pin_thick_multiplier" args \
    keys {-hor_multiplier -ver_multiplier} flags {}

  sta::check_argc_eq0 "set_pin_thick_multiplier" $args

  if { [info exists keys(-hor_multiplier)] } {
    ppl::set_hor_thick_multiplier $keys(-hor_multiplier)
  }

  if { [info exists keys(-ver_multiplier)] } {
    ppl::set_ver_thick_multiplier $keys(-ver_multiplier)
  }
}

sta::define_cmd_args "set_simulated_annealing" {[-temperature temperature]\
                                                [-max_iterations iters]\
                                                [-perturb_per_iter perturbs]\
                                                [-alpha alpha]
}

proc set_simulated_annealing { args } {
  sta::parse_key_args "set_simulated_annealing" args \
    keys {-temperature -max_iterations -perturb_per_iter -alpha} flags {}

  set temperature 0
  if { [info exists keys(-temperature)] } {
    set temperature $keys(-temperature)
    sta::check_positive_float "-temperature" $temperature
  }

  set max_iterations 0
  if { [info exists keys(-max_iterations)] } {
    set max_iterations $keys(-max_iterations)
    sta::check_positive_integer "-max_iterations" $max_iterations
  }

  set perturb_per_iter 0
  if { [info exists keys(-perturb_per_iter)] } {
    set perturb_per_iter $keys(-perturb_per_iter)
    sta::check_positive_integer "-perturb_per_iter" $perturb_per_iter
  }

  set alpha 0
  if { [info exists keys(-alpha)] } {
    set alpha $keys(-alpha)
    sta::check_positive_float "-alpha" $alpha
  }

  ppl::set_simulated_annealing $temperature $max_iterations $perturb_per_iter $alpha
}

sta::define_cmd_args "simulated_annealing_debug" {
  [-iters_between_paintings iters]
  [-no_pause_mode no_pause_mode]
}

proc simulated_annealing_debug { args } {
  sta::parse_key_args "simulated_annealing_debug" args \
    keys {-iters_between_paintings} \
    flags {-no_pause_mode}

  if { [info exists keys(-iters_between_paintings)] } {
    set iters $keys(-iters_between_paintings)
    sta::check_positive_integer "-iters_between_paintings" $iters
    ppl::simulated_annealing_debug $iters [info exists flags(-no_pause_mode)]
  } else {
    utl::error PPL 108 "The -iters_between_paintings argument is required when using debug."
  }
}

sta::define_cmd_args "place_pin" {[-pin_name pin_name]\
                                  [-layer layer]\
                                  [-location location]\
                                  [-pin_size pin_size]\
                                  [-force_to_die_boundary]\
                                  [-placed_status]
}

proc place_pin { args } {
  sta::parse_key_args "place_pin" args \
    keys {-pin_name -layer -location -pin_size} \
    flags {-force_to_die_boundary -placed_status}

  sta::check_argc_eq0 "place_pin" $args

  if { [info exists keys(-pin_name)] } {
    set pin_name $keys(-pin_name)
  } else {
    utl::error PPL 64 "-pin_name is required."
  }

  if { [info exists keys(-layer)] } {
    set layer $keys(-layer)
  } else {
    utl::error PPL 65 "-layer is required."
  }

  set tech_layer [ppl::parse_layer_name $layer]
  set layer_direction [$tech_layer getDirection]
  if {
    ($layer_direction == "HORIZONTAL" && ![ord::db_layer_has_hor_tracks $tech_layer])
    || ($layer_direction == "VERTICAL" && ![ord::db_layer_has_ver_tracks $tech_layer])
  } {
    utl::error PPL 22 "Routing tracks not found for layer $layer."
  }

  if {
    ($layer_direction == "VERTICAL" && ![ord::db_layer_has_hor_tracks $tech_layer])
    || ($layer_direction == "HORIZONTAL" && ![ord::db_layer_has_ver_tracks $tech_layer])
  } {
    utl::warn PPL 10 \
      "Routing tracks in the non-preferred direction were not found for the layer $layer."
  }

  if { [info exists keys(-location)] } {
    set location $keys(-location)
  } else {
    utl::error PPL 66 "-location is required."
  }

  if { [llength $location] != 2 } {
    utl::error PPL 68 "-location is not a list of 2 values."
  }
  lassign $location x y
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]

  if { [info exists keys(-pin_size)] } {
    set pin_size $keys(-pin_size)
  } else {
    set pin_size {0 0}
  }

  if { [llength $pin_size] != 2 } {
    utl::error PPL 69 "-pin_size is not a list of 2 values."
  }
  lassign $pin_size width height
  set width [ord::microns_to_dbu $width]
  set height [ord::microns_to_dbu $height]

  set pin [ppl::parse_pin_names "place_pin" $pin_name]
  if { [llength $pin] > 1 } {
    utl::error PPL 71 "Command place_pin should receive only one pin name."
  }

  set layer [ppl::parse_layer_name $layer]

  ppl::place_pin $pin $layer $x $y $width $height \
    [info exists flags(-force_to_die_boundary)] \
    [info exists flags(-placed_status)]
}

sta::define_cmd_args "write_pin_placement" { file_name \
                                             [-placed_status] }

proc write_pin_placement { args } {
  sta::parse_key_args "write_pin_placement" args \
    keys {} flags {-placed_status}
  set file_name $args
  ppl::write_pin_placement $file_name [info exists flags(-placed_status)]
}

sta::define_cmd_args "place_pins" {[-hor_layers h_layers]\
                                  [-ver_layers v_layers]\
                                  [-random_seed seed]\
                                  [-random]\
                                  [-corner_avoidance distance]\
                                  [-min_distance min_dist]\
                                  [-min_distance_in_tracks]\
                                  [-exclude region]\
                                  [-group_pins pin_list]\
                                  [-annealing] \
                                  [-minimize_displacement] \
                                  [-write_pin_placement file_name]
}

proc place_pins { args } {
  ord::parse_list_args "place_pins" args list {-exclude -group_pins}
  sta::parse_key_args "place_pins" args \
    keys {-hor_layers -ver_layers -random_seed -corner_avoidance \
          -min_distance -write_pin_placement} \
    flags {-random -min_distance_in_tracks -annealing -minimize_displacement}

  sta::check_argc_eq0 "place_pins" $args

  if { [info exists flags(-random)] || [info exists keys(-random_seed)] } {
    utl::warn PPL 113 "-random and -random_seed are obsolete. Skipping random pin placement."
  } else {
    set regions $list(-exclude)
    set pin_groups $list(-group_pins)

    set dbTech [ord::get_db_tech]
    if { $dbTech == "NULL" } {
      utl::error PPL 31 "No technology found."
    }

    set dbBlock [ord::get_db_block]
    if { $dbBlock == "NULL" } {
      utl::error PPL 32 "No block found."
    }

    set db [ord::get_db]

    set blockages {}

    foreach inst [$dbBlock getInsts] {
      if { [$inst isBlock] } {
        if { ![$inst isPlaced] } {
          utl::warn PPL 15 "Macro [$inst getName] is not placed."
        } else {
          lappend blockages $inst
        }
      }
    }

    utl::report "Found [llength $blockages] macro blocks."

    # set default interval_length from boundaries as 1u
    set distance 1
    if { [info exists keys(-corner_avoidance)] } {
      set distance $keys(-corner_avoidance)
      ppl::set_corner_avoidance [ord::microns_to_dbu $distance]
    }

    set min_dist 2
    set dist_in_tracks [info exists flags(-min_distance_in_tracks)]
    ppl::check_min_distance_in_tracks $dist_in_tracks \
      [info exists keys(-min_distance)]
    if { [info exists keys(-min_distance)] } {
      set min_dist $keys(-min_distance)
      if { $dist_in_tracks } {
        ppl::set_min_distance $min_dist
      } else {
        ppl::set_min_distance [ord::microns_to_dbu $min_dist]
      }
    } else {
      if { [ppl::get_default_min_distance] == 0 } {
        utl::report "Using $min_dist tracks default min distance between IO pins."
      }
      # 0 means unset: the run uses set_place_config's value or the default
      ppl::set_min_distance 0
    }
    ppl::set_min_distance_in_tracks $dist_in_tracks

    set bterms_cnt [llength [$dbBlock getBTerms]]

    if { $bterms_cnt == 0 } {
      utl::error PPL 19 "Design without pins."
    }

    # Switches override set_place_config for this run only.
    set hor_layers {}
    set ver_layers {}
    if { [info exists keys(-hor_layers)] } {
      set hor_layers $keys(-hor_layers)
    }
    if { [info exists keys(-ver_layers)] } {
      set ver_layers $keys(-ver_layers)
    }
    set has_hor [expr { $hor_layers != {} || [ppl::has_default_layers true] }]
    set has_ver [expr { $ver_layers != {} || [ppl::has_default_layers false] }]
    if { !$has_hor && !$has_ver } {
      utl::error PPL 26 "Both horizontal and vertical pin layers are required.\
        Pass -hor_layers and -ver_layers, or set_place_config\
        -io_pin_hor_layers and -io_pin_ver_layers."
    }
    if { !$has_hor } {
      utl::error PPL 17 "-hor_layers or set_place_config -io_pin_hor_layers\
        is required."
    }
    if { !$has_ver } {
      utl::error PPL 18 "-ver_layers or set_place_config -io_pin_ver_layers\
        is required."
    }
    ppl::set_io_pin_layers $hor_layers $ver_layers

    if { [llength $regions] != 0 } {
      set lef_units [$dbTech getLefUnits]

      foreach region $regions {
        exclude_io_pin_region -region $region
      }
    }

    if { [llength $pin_groups] != 0 } {
      foreach group $pin_groups {
        set pins [ppl::parse_pin_names "place_pins -group_pins" $group]
        if { [llength $pins] != 0 } {
          odb::add_pin_group $pins 0
        }
      }
    }

    if { [info exists keys(-write_pin_placement)] } {
      ppl::set_pin_placement_file $keys(-write_pin_placement)
    }

    set minimize_displacement [info exists flags(-minimize_displacement)]
    if { [info exists flags(-annealing)] } {
      ppl::run_annealing $minimize_displacement
    } else {
      ppl::run_hungarian_matching $minimize_displacement
    }
  }
}

namespace eval ppl {
proc check_min_distance_in_tracks { in_tracks has_min_distance } {
  if { $in_tracks && !$has_min_distance } {
    utl::error PPL 37 "-min_distance_in_tracks requires -min_distance."
  }
}

# Checks every layer before adding any, so an error leaves no partial set.
proc set_io_pin_layers { hor_layers ver_layers } {
  foreach { dir layer } [check_io_pin_layers $hor_layers $ver_layers] {
    ppl::add_${dir}_layer $layer
  }
}

proc check_io_pin_layers { hor_layers ver_layers } {
  set layers {}
  foreach name $hor_layers {
    set layer [ppl::parse_layer_name $name]
    if { ![ord::db_layer_has_hor_tracks $layer] } {
      utl::error PPL 21 "Horizontal routing tracks not found for layer $name."
    }
    if { [$layer getDirection] != "HORIZONTAL" } {
      utl::error PPL 45 "Layer $name preferred direction is not horizontal."
    }
    lappend layers hor $layer
  }
  foreach name $ver_layers {
    set layer [ppl::parse_layer_name $name]
    if { ![ord::db_layer_has_ver_tracks $layer] } {
      utl::error PPL 23 "Vertical routing tracks not found for layer $name."
    }
    if { [$layer getDirection] != "VERTICAL" } {
      utl::error PPL 46 "Layer $name preferred direction is not vertical."
    }
    lappend layers ver $layer
  }
  return $layers
}

proc parse_edge { cmd edge } {
  if {
    $edge != "top" && $edge != "bottom" &&
    $edge != "left" && $edge != "right"
  } {
    utl::error PPL 27 "$cmd: $edge is an invalid edge. Use top, bottom, left or right."
  }
  return [ppl::get_edge $edge]
}

proc parse_direction { cmd direction } {
  if {
    [regexp -nocase -- {^INPUT$} $direction] ||
    [regexp -nocase -- {^OUTPUT$} $direction] ||
    [regexp -nocase -- {^INOUT$} $direction] ||
    [regexp -nocase -- {^FEEDTHRU$} $direction]
  } {
    set direction [string tolower $direction]
    return [ppl::get_direction $direction]
  } else {
    utl::error PPL 28 "$cmd: Invalid pin direction."
  }
}

proc get_edge_extreme { cmd begin edge } {
  set dbBlock [ord::get_db_block]
  set die_area [$dbBlock getDieArea]
  if { $begin } {
    if { $edge == "top" || $edge == "bottom" } {
      set extreme [$die_area xMin]
    } elseif { $edge == "left" || $edge == "right" } {
      set extreme [$die_area yMin]
    } else {
      utl::error PPL 29 "$cmd: Invalid edge"
    }
  } else {
    if { $edge == "top" || $edge == "bottom" } {
      set extreme [$die_area xMax]
    } elseif { $edge == "left" || $edge == "right" } {
      set extreme [$die_area yMax]
    } else {
      utl::error PPL 30 "Invalid edge for command $cmd, should be one of top, bottom, left, right."
    }
  }

  return $extreme
}

proc exclude_intervals { cmd intervals } {
  if { $intervals != {} } {
    foreach interval $intervals {
      ppl::exclude_interval $interval
    }
  }
}

proc parse_layer_name { layer_name } {
  if { ![ord::db_has_tech] } {
    utl::error PPL 50 "No technology has been read."
  }
  set tech [ord::get_db_tech]
  set tech_layer [$tech findLayer $layer_name]
  if { $tech_layer == "NULL" } {
    utl::error PPL 51 "Layer $layer_name not found."
  }
  return $tech_layer
}

proc parse_pin_names { cmd names } {
  set dbBlock [ord::get_db_block]
  set pin_list {}
  foreach pin [get_ports $names] {
    lappend pin_list [sta::sta_to_db_port $pin]
  }

  if { [llength $pin_list] == 0 } {
    utl::error PPL 61 "Pins {$names} for $cmd command were not found."
  }

  return $pin_list
}

# ppl namespace end
}

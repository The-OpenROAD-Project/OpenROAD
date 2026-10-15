# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2018-2025, The OpenROAD Authors

# The switches are generated from gpl::PlaceOptions (C++): one per field,
# -timing.nets_percentage for timing.nets_percentage, a flag for a bool.
# The older names (-timing_driven, -init_density_penalty, ...) are aliases.
# See `describe_options global_placement`.
sta::define_cmd_args "global_placement" [concat {\
    [-skip_nesterov_place]\
    [-incremental]} [utl::options_synopsis_cmd global_placement]]

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {} \
    flags {-skip_nesterov_place -incremental} 0

  set skip_nesterov_place [info exists flags(-skip_nesterov_place)]
  if { [info exists flags(-incremental)] } {
    gpl::replace_incremental_place_cmd $args
  } else {
    gpl::replace_initial_place_cmd $args $skip_nesterov_place

    if { !$skip_nesterov_place } {
      gpl::replace_nesterov_place_cmd $args
    }
  }
  gpl::replace_reset_cmd
}

# The settings live in block properties, so they persist with the design.
# place_pins and global_placement -place_ios read the IO pin settings.
sta::define_cmd_args "set_place_config" {[-io_pin_hor_layers h_layers]\
                                         [-io_pin_ver_layers v_layers]\
                                         [-io_pin_corner_avoidance distance]\
                                         [-io_pin_min_distance min_dist]\
                                         [-io_pin_min_distance_in_tracks]}

proc set_place_config { args } {
  sta::parse_key_args "set_place_config" args \
    keys {-io_pin_hor_layers -io_pin_ver_layers -io_pin_corner_avoidance -io_pin_min_distance} \
    flags {-io_pin_min_distance_in_tracks}
  sta::check_argc_eq0 "set_place_config" $args

  set block [gpl::place_config_block]
  set in_tracks [info exists flags(-io_pin_min_distance_in_tracks)]
  if { $in_tracks && ![info exists keys(-io_pin_min_distance)] } {
    utl::error GPL 192 \
      "-io_pin_min_distance_in_tracks requires -io_pin_min_distance."
  }

  set hor_layers {}
  set ver_layers {}
  if { [info exists keys(-io_pin_hor_layers)] } {
    set hor_layers $keys(-io_pin_hor_layers)
  }
  if { [info exists keys(-io_pin_ver_layers)] } {
    set ver_layers $keys(-io_pin_ver_layers)
  }
  ppl::check_io_pin_layers $hor_layers $ver_layers
  if { [info exists keys(-io_pin_hor_layers)] } {
    gpl::set_place_prop $block String io_pin_hor_layers $hor_layers
  }
  if { [info exists keys(-io_pin_ver_layers)] } {
    gpl::set_place_prop $block String io_pin_ver_layers $ver_layers
  }
  if { [info exists keys(-io_pin_corner_avoidance)] } {
    gpl::set_place_prop $block Int io_pin_corner_avoidance \
      [ord::microns_to_dbu $keys(-io_pin_corner_avoidance)]
  }
  if { [info exists keys(-io_pin_min_distance)] } {
    set min_dist $keys(-io_pin_min_distance)
    if { !$in_tracks } {
      set min_dist [ord::microns_to_dbu $min_dist]
    }
    gpl::set_place_prop $block Int io_pin_min_distance $min_dist
    gpl::set_place_prop $block Bool io_pin_min_distance_in_tracks $in_tracks
  }
}

sta::define_cmd_args "reset_place_config" {[-io_pin_hor_layers]\
                                           [-io_pin_ver_layers]\
                                           [-io_pin_corner_avoidance]\
                                           [-io_pin_min_distance]}

proc reset_place_config { args } {
  sta::parse_key_args "reset_place_config" args \
    keys {} \
    flags {-io_pin_hor_layers -io_pin_ver_layers -io_pin_corner_avoidance \
      -io_pin_min_distance}
  sta::check_argc_eq0 "reset_place_config" $args

  set block [gpl::place_config_block]
  set reset_all [expr { [array size flags] == 0 }]
  foreach { flag props } {
    -io_pin_hor_layers {String io_pin_hor_layers}
    -io_pin_ver_layers {String io_pin_ver_layers}
    -io_pin_corner_avoidance {Int io_pin_corner_avoidance}
    -io_pin_min_distance {Int io_pin_min_distance Bool io_pin_min_distance_in_tracks}
  } {
    if { $reset_all || [info exists flags($flag)] } {
      foreach { type name } $props {
        gpl::clear_place_prop $block $type $name
      }
    }
  }
}

sta::define_cmd_args "report_place_config" {}

proc report_place_config { args } {
  sta::parse_key_args "report_place_config" args keys {} flags {}
  sta::check_argc_eq0 "report_place_config" $args

  set block [gpl::place_config_block]
  foreach name {io_pin_hor_layers io_pin_ver_layers} {
    set value [gpl::get_place_prop $block String $name]
    utl::report "$name: [expr { $value eq {} ? {unset} : $value }]"
  }
  set corner [gpl::get_place_prop $block Int io_pin_corner_avoidance]
  if { $corner eq {} } {
    utl::report "io_pin_corner_avoidance: unset"
  } else {
    utl::report "io_pin_corner_avoidance: [ord::dbu_to_microns $corner] um"
  }
  set min_dist [gpl::get_place_prop $block Int io_pin_min_distance]
  if { $min_dist eq {} } {
    utl::report "io_pin_min_distance: unset"
  } elseif { [gpl::get_place_prop $block Bool io_pin_min_distance_in_tracks] } {
    utl::report "io_pin_min_distance: $min_dist tracks"
  } else {
    utl::report "io_pin_min_distance: [ord::dbu_to_microns $min_dist] um"
  }
}

sta::define_cmd_args "cluster_flops" {\
    [-tray_weight tray_weight]\
    [-timing_weight timing_weight]\
    [-max_split_size max_split_size]\
    [-num_paths num_paths]\
    [-clock_power_weight clock_power_weight]\
}

proc cluster_flops { args } {
  sta::parse_key_args "cluster_flops" args \
    keys { -tray_weight -timing_weight -max_split_size -num_paths \
      -clock_power_weight } \
    flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 113 "No design block found."
  }

  set tray_weight 32.0
  set timing_weight 0.1
  set max_split_size 500
  set num_paths 0
  set clock_power_weight 0.0

  if { [info exists keys(-tray_weight)] } {
    set tray_weight $keys(-tray_weight)
  }

  if { [info exists keys(-timing_weight)] } {
    set timing_weight $keys(-timing_weight)
  }

  if { [info exists keys(-max_split_size)] } {
    set max_split_size $keys(-max_split_size)
  }

  if { [info exists keys(-num_paths)] } {
    set num_paths $keys(-num_paths)
  }

  if { [info exists keys(-clock_power_weight)] } {
    # Non-negativity is enforced in MBFF::Run so that non-Tcl callers
    # (Python, direct C++) are validated too; see src/gpl/src/mbff.cpp.
    set clock_power_weight $keys(-clock_power_weight)
  }

  gpl::replace_run_mbff_cmd $max_split_size $tray_weight $timing_weight \
    $num_paths $clock_power_weight
}

sta::define_cmd_args "global_placement_debug" {
  [-pause pause]
  [-update update]
  [-inst inst]
  [-pin pin]
  [-start_iter start_iter]
  [-start_rudy start_rudy]
  [-rudy_stride rudy_stride]
  [-images_path images_path]
  [-draw_bins]
  [-initial]
  [-generate_images]
}

proc global_placement_debug { args } {
  sta::parse_key_args "global_placement_debug" args \
    keys {-pause -update -inst -pin -start_iter -images_path \
      -start_rudy -rudy_stride} \
    flags {-draw_bins -initial -generate_images}

  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 117 "No design block found."
  }

  set pause 10
  if { [info exists keys(-pause)] } {
    set pause $keys(-pause)
    sta::check_positive_integer "-pause" $pause
  }

  set update 10
  if { [info exists keys(-update)] } {
    set update $keys(-update)
    sta::check_positive_integer "-update" $update
  }

  set inst ""
  if { [info exists keys(-inst)] } {
    set inst $keys(-inst)
  }

  set pin ""
  if { [info exists keys(-pin)] } {
    set pin $keys(-pin)
  }

  set start_iter 0
  if { [info exists keys(-start_iter)] } {
    set start_iter $keys(-start_iter)
    sta::check_positive_integer "-start_iter" $start_iter
  }

  set start_rudy 0
  if { [info exists keys(-start_rudy)] } {
    set start_rudy $keys(-start_rudy)
    sta::check_positive_integer "-start_rudy" $start_rudy
  }

  set rudy_stride 1
  if { [info exists keys(-rudy_stride)] } {
    set rudy_stride $keys(-rudy_stride)
    sta::check_positive_integer "-rudy_stride" $rudy_stride
  }

  set draw_bins [info exists flags(-draw_bins)]
  set initial [info exists flags(-initial)]
  set generate_images [info exists flags(-generate_images)]

  set images_path ""
  if { [info exists keys(-images_path)] } {
    set images_path $keys(-images_path)
  }

  gpl::set_debug_cmd $pause $update $draw_bins $initial \
    $inst $pin $start_iter $start_rudy $rudy_stride $generate_images $images_path
}

sta::define_cmd_args "placement_cluster" {}

proc placement_cluster { args } {
  sta::parse_key_args "placement_cluster" args \
    keys {} \
    flags {}

  if { $args == {} } {
    utl::error GPL 94 "placement_cluster requires a list of instances."
  }

  if { [llength $args] == 1 } {
    set args [lindex $args 0]
  }

  set insts []
  foreach inst_name $args {
    lappend insts {*}[gpl::parse_inst_names placement_cluster $inst_name]
  }
  utl::info GPL 96 "Created placement cluster of [llength $insts] instances."

  gpl::placement_cluster_cmd $insts
}

sta::define_cmd_args "estimate_target_density" {\
    [-bin_grid_count grid_count]\
    [-overflow overflow]\
    [-pad_left pad_left]\
    [-pad_right pad_right]\
}

# Takes the switches of global_placement; the ones listed matter.
proc estimate_target_density { args } {
  sta::parse_key_args "estimate_target_density" args \
    keys {} \
    flags {} 0

  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 187 "No design block found."
  }

  set density_estimation [gpl::estimate_target_density_cmd $args]
  gpl::replace_reset_cmd

  return $density_estimation
}

namespace eval gpl {
proc place_config_block { } {
  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GPL 194 "No design block found."
  }
  return $block
}

# type is String, Int or Bool. The property names are read by ppl.
proc set_place_prop { block type name value } {
  set prop [odb::db${type}Property_find $block "place_config_$name"]
  if { $prop eq "NULL" } {
    odb::db${type}Property_create $block "place_config_$name" $value
  } else {
    $prop setValue $value
  }
}

proc get_place_prop { block type name } {
  set prop [odb::db${type}Property_find $block "place_config_$name"]
  if { $prop eq "NULL" } {
    return {}
  }
  return [$prop getValue]
}

proc clear_place_prop { block type name } {
  set prop [odb::db${type}Property_find $block "place_config_$name"]
  if { $prop ne "NULL" } {
    odb::dbProperty_destroy $prop
  }
}

proc get_global_placement_uniform_density { args } {
  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 114 "No design block found."
  }

  sta::parse_key_args "get_global_placement_uniform_density" args \
    keys { -pad_left -pad_right } \
    flags {} ;# checker off

  set switches {}
  foreach key [array names keys] {
    lappend switches $key $keys($key)
  }

  set uniform_density 0
  if { [ord::db_has_core_rows] } {
    sta::check_argc_eq0 "get_global_placement_uniform_density" $args
    set uniform_density [gpl::get_global_placement_uniform_density_cmd $switches]
    gpl::replace_reset_cmd
  } else {
    utl::error GPL 131 "No rows defined in design. Use initialize_floorplan to add rows."
  }
  return $uniform_density
}

proc parse_inst_names { cmd names } {
  set dbBlock [ord::get_db_block]
  set inst_list {}
  foreach inst [get_cells $names] {
    lappend inst_list [sta::sta_to_db_inst $inst]
  }

  if { [llength $inst_list] == 0 } {
    utl::error GPL 95 "Instances {$names} for $cmd command were not found."
  }

  return $inst_list
}
}

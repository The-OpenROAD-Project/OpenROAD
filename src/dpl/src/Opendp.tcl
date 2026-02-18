# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

sta::define_cmd_args "detailed_placement" { \
                           [-max_displacement disp|{disp_x disp_y}] \
                           [-disallow_one_site_gaps] \
                           [-report_file_name file_name]}

proc detailed_placement { args } {
  sta::parse_key_args "detailed_placement" args \
    keys {-max_displacement -report_file_name} flags {-disallow_one_site_gaps}

  if { [info exists keys(-max_displacement)] } {
    set max_displacement $keys(-max_displacement)
    if { [llength $max_displacement] == 1 } {
      sta::check_positive_integer "-max_displacement" $max_displacement
      set max_displacement_x $max_displacement
      set max_displacement_y $max_displacement
    } elseif { [llength $max_displacement] == 2 } {
      lassign $max_displacement max_displacement_x max_displacement_y
      sta::check_positive_integer "-max_displacement" $max_displacement_x
      sta::check_positive_integer "-max_displacement" $max_displacement_y
    } else {
      sta::error DPL 31 "-max_displacement disp|{disp_x disp_y}"
    }
  } else {
    # use default displacement
    set max_displacement_x 0
    set max_displacement_y 0
  }
  set file_name ""
  if { [info exists keys(-report_file_name)] } {
    set file_name $keys(-report_file_name)
  }

  sta::check_argc_eq0 "detailed_placement" $args

  if { [info exists flags(-disallow_one_site_gaps)] } {
    utl::warn DPL 3 "-disallow_one_site_gaps is deprecated"
  }

  if { [ord::db_has_core_rows] } {
    set site [dpl::get_row_site]
    # Convert displacement from microns to sites.
    set max_displacement_x [expr [ord::microns_to_dbu $max_displacement_x] \
      / [$site getWidth]]
    set max_displacement_y [expr [ord::microns_to_dbu $max_displacement_y] \
      / [$site getHeight]]
    dpl::detailed_placement_cmd $max_displacement_x $max_displacement_y \
      $file_name
    dpl::report_legalization_stats
  } else {
    utl::error "DPL" 27 "no rows defined in design. Use initialize_floorplan to add rows."
  }
}

sta::define_cmd_args "set_placement_padding" { -global|-masters masters|-instances insts\
                                                 [-right site_count]\
                                                 [-left site_count] \
                                                 [instances]\
                                               }

proc set_placement_padding { args } {
  sta::parse_key_args "set_placement_padding" args \
    keys {-masters -instances -right -left} flags {-global}

  set left 0
  if { [info exists keys(-left)] } {
    set left $keys(-left)
    sta::check_positive_integer "-left" $left
  }
  set right 0
  if { [info exists keys(-right)] } {
    set right $keys(-right)
    sta::check_positive_integer "-right" $right
  }

  sta::check_argc_eq0 "set_placement_padding" $args
  if { [info exists flags(-global)] } {
    dpl::set_padding_global $left $right
  } elseif { [info exists keys(-masters)] } {
    set masters [dpl::get_masters_arg "-masters" $keys(-masters)]
    foreach master $masters {
      dpl::set_padding_master $master $left $right
    }
  } elseif { [info exists keys(-instances)] } {
    # sta::get_instances_error supports sdc get_cells
    set insts [sta::get_instances_error "-instances" $keys(-instances)]
    foreach inst $insts {
      set db_inst [sta::sta_to_db_inst $inst]
      dpl::set_padding_inst $db_inst $left $right
    }
  }
}

sta::define_cmd_args "filler_placement" { [-prefix prefix] [-verbose] filler_masters }

proc filler_placement { args } {
  sta::parse_key_args "filler_placement" args \
    keys {-prefix} flags {-verbose}

  set prefix "FILLER_"
  if { [info exists keys(-prefix)] } {
    set prefix $keys(-prefix)
  }

  sta::check_argc_eq1 "filler_placement" $args
  set filler_masters [dpl::get_masters_arg "filler_masters" [lindex $args 0]]
  dpl::filler_placement_cmd $filler_masters $prefix [info exists flags(-verbose)]
}

sta::define_cmd_args "remove_fillers" {}

proc remove_fillers { args } {
  sta::parse_key_args "remove_fillers" args keys {} flags {}
  sta::check_argc_eq0 "remove_fillers" $args
  if { [ord::get_db_block] == "NULL" } {
    utl::error DPL 105 "No design block found."
  }
  dpl::remove_fillers_cmd
}

sta::define_cmd_args "check_placement" {[-verbose] \
                                        [-disallow_one_site_gaps] \
                                        [-report_file_name file_name]}

proc check_placement { args } {
  if { [ord::get_db_block] == "NULL" } {
    utl::error DPL 103 "No design block found."
  }

  sta::parse_key_args "check_placement" args \
    keys {-report_file_name} flags {-verbose -disallow_one_site_gaps}
  set verbose [info exists flags(-verbose)]
  sta::check_argc_eq0 "check_placement" $args
  set file_name ""
  if { [info exists keys(-report_file_name)] } {
    set file_name $keys(-report_file_name)
  }
  if { [info exists flags(-disallow_one_site_gaps)] } {
    utl::warn DPL 4 "-disallow_one_site_gaps is deprecated"
  }
  dpl::check_placement_cmd $verbose $file_name
}

sta::define_cmd_args "optimize_mirroring" {}

proc optimize_mirroring { args } {
  sta::parse_key_args "optimize_mirroring" args keys {} flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error DPL 104 "No design block found."
  }

  sta::check_argc_eq0 "optimize_mirroring" $args
  dpl::optimize_mirroring_cmd
}

sta::define_cmd_args "improve_placement" {\
    [-random_seed seed]\
    [-max_displacement disp|{disp_x disp_y}]\
    [-global_swap_args {options}]\
    [-enable_extra_dpl bool]\
    [-disallow_one_site_gaps]\
}

proc improve_placement { args } {
  sta::parse_key_args "improve_placement" args \
    keys {-random_seed -max_displacement -global_swap_args -enable_extra_dpl} \
    flags {-disallow_one_site_gaps}

  if { [ord::get_db_block] == "NULL" } {
    utl::error DPL 342 "No design block found."
  }

  if { [info exists flags(-disallow_one_site_gaps)] } {
    utl::warn DPL 343"-disallow_one_site_gaps is deprecated"
  }
  set seed 1
  if { [info exists keys(-random_seed)] } {
    set seed $keys(-random_seed)
  }
  if { [info exists keys(-max_displacement)] } {
    set max_displacement $keys(-max_displacement)
    if { [llength $max_displacement] == 1 } {
      sta::check_positive_integer "-max_displacement" $max_displacement
      set max_displacement_x $max_displacement
      set max_displacement_y $max_displacement
    } elseif { [llength $max_displacement] == 2 } {
      lassign $max_displacement max_displacement_x max_displacement_y
      sta::check_positive_integer "-max_displacement" $max_displacement_x
      sta::check_positive_integer "-max_displacement" $max_displacement_y
    } else {
      sta::error DPL 344 "-max_displacement disp|{disp_x disp_y}"
    }
  } else {
    # use default displacement
    set max_displacement_x 0
    set max_displacement_y 0
  }

  dpl::reset_global_swap_params_cmd
  if { [info exists keys(-global_swap_args)] } {
    set global_swap_passes -1
    set global_swap_tolerance -1
    set global_swap_tradeoff -1
    set global_swap_area_weight -1
    set global_swap_pin_weight -1
    set global_swap_user_weight -1
    set global_swap_sampling -1
    set global_swap_normalization -1
    set global_swap_profiling_excess -1
    set global_swap_budget_list {}

    set global_swap_args $keys(-global_swap_args)
    if { ([llength $global_swap_args] % 2) != 0 } {
      sta::error DPL 345 "-global_swap_args must be key/value pairs"
    }
    foreach {opt value} $global_swap_args {
      switch -- $opt {
        -passes {
          set global_swap_passes $value
        }
        -tolerance {
          set global_swap_tolerance $value
        }
        -tradeoff {
          set global_swap_tradeoff $value
        }
        -area_weight {
          set global_swap_area_weight $value
        }
        -pin_weight {
          set global_swap_pin_weight $value
        }
        -congestion_user_weight {
          set global_swap_user_weight $value
        }
        -sampling_moves {
          set global_swap_sampling $value
        }
        -normalization_interval {
          set global_swap_normalization $value
        }
        -profiling_excess {
          set global_swap_profiling_excess $value
        }
        -budget_multipliers {
          set global_swap_budget_list {}
          foreach multiplier $value {
            set trimmed [string trim $multiplier]
            if { $trimmed eq "" } {
              continue
            }
            if { [catch { expr { double($trimmed) } } parsed] } {
              sta::error DPL 347 "Invalid -budget_multipliers value \"$multiplier\""
            }
            lappend global_swap_budget_list $parsed
          }
        }
        default {
          sta::error DPL 346 "Unknown -global_swap_args option $opt"
        }
      }
    }
    set global_swap_budget_str ""
    if { [llength $global_swap_budget_list] > 0 } {
      set global_swap_budget_str [join $global_swap_budget_list " "]
    }
    dpl::configure_global_swap_params_cmd \
      $global_swap_passes \
      $global_swap_tolerance \
      $global_swap_tradeoff \
      $global_swap_area_weight \
      $global_swap_pin_weight \
      $global_swap_user_weight \
      $global_swap_sampling \
      $global_swap_normalization \
      $global_swap_profiling_excess \
      $global_swap_budget_str
  }

  set extra_dpl_enabled 0
  if { [info exists keys(-enable_extra_dpl)] } {
    set extra_dpl_enabled $keys(-enable_extra_dpl)
  } elseif { [info exists ::env(ENABLE_EXTRA_DPL)] } {
    set extra_dpl_enabled $::env(ENABLE_EXTRA_DPL)
  }
  set extra_dpl_enabled [expr { $extra_dpl_enabled ? 1 : 0 }]
  dpl::set_extra_dpl_cmd $extra_dpl_enabled

  sta::check_argc_eq0 "improve_placement" $args
  dpl::improve_placement_cmd $seed $max_displacement_x $max_displacement_y
}
namespace eval dpl {
# min_displacement is the smallest displacement to draw
# measured as a multiple of row_height.
proc detailed_placement_debug { args } {
  sta::parse_key_args "detailed_placement_debug" args \
    keys {-instance -min_displacement -jump_moves} flags {-iterative}


  if { [info exists keys(-min_displacement)] } {
    set min_displacement $keys(-min_displacement)
  } else {
    set min_displacement 0
  }

  set jump_moves 0
  if { [info exists keys(-jump_moves)] } {
    set jump_moves $keys(-jump_moves)
    sta::check_positive_integer "-jump_moves" $jump_moves
  }

  if { [info exists keys(-instance)] } {
    set instance_name $keys(-instance)
    set block [ord::get_db_block]
    set debug_instance [$block findInst $instance_name]
    if { $debug_instance == "NULL" } {
      utl::error DPL 32 "Debug instance $instance_name not found."
    }
  } else {
    set debug_instance "NULL"
  }

  dpl::set_debug_cmd $min_displacement $debug_instance $jump_moves [info exists flags(-iterative)]
}

proc get_masters_arg { arg_name arg } {
  set masters {}
  # Expand master name regexps
  set db [ord::get_db]
  foreach name $arg {
    set matched 0
    foreach lib [$db getLibs] {
      foreach master [$lib getMasters] {
        set master_name [$master getConstName]
        if { [string match $name $master_name] } {
          lappend masters $master
          set matched 1
        }
      }
    }
    if { !$matched } {
      utl::warn "DPL" 28 "$name did not match any masters."
    }
  }
  if { [llength $arg] > 0 && [llength $masters] == 0 } {
    utl::error "DPL" 39 "\"$arg\" did not match any masters.
This could be due to a change from using regex to glob to search for cell masters. https://github.com/The-OpenROAD-Project/OpenROAD/pull/3210"
  }
  return $masters
}

proc get_inst_bbox { inst_name } {
  set block [ord::get_db_block]
  set inst [$block findInst $inst_name]
  if { $inst != "NULL" } {
    set bbox [$inst getBBox]
    return "[$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]"
  } else {
    utl::error "DPL" 29 "cannot find instance $inst_name"
  }
}

proc get_inst_grid_bbox { inst_name } {
  set block [ord::get_db_block]
  set inst [$block findInst $inst_name]
  set rows [$block getRows]
  set site [[lindex $rows 0] getSite]
  set width [$site getWidth]
  set height [$site getHeight]
  if { $inst != "NULL" } {
    set bbox [$inst getBBox]
    return "[format_grid [$bbox xMin] $width] [format_grid [$bbox yMin] $height]\
            [format_grid [$bbox xMax] $width] [format_grid [$bbox yMax] $height]"
  } else {
    utl::error "DPL" 30 "cannot find instance $inst_name"
  }
}

proc format_grid { x w } {
  if { $x % $w == 0 } {
    return [expr $x / $w]
  } else {
    return [format "%.2f" [expr $x / double($w)]]
  }
}

proc get_row_site { } {
  return [[lindex [[ord::get_db_block] getRows] 0] getSite]
}
}

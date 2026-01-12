# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2018-2025, The OpenROAD Authors

sta::define_cmd_args "global_placement" {\
    [-skip_initial_place]\
    [-skip_nesterov_place]\
    [-timing_driven]\
    [-routability_driven]\
    [-disable_timing_driven]\
    [-disable_routability_driven]\
    [-incremental]\
    [-skip_io]\
    [-random_seed random_seed]\
    [-bin_grid_count grid_count]\
    [-density target_density]\
    [-init_density_penalty init_density_penalty]\
    [-init_wirelength_coef init_wirelength_coef]\
    [-min_phi_coef min_phi_coef]\
    [-max_phi_coef max_phi_coef]\
    [-reference_hpwl reference_hpwl]\
    [-overflow overflow]\
    [-initial_place_max_iter initial_place_max_iter]\
    [-initial_place_max_fanout initial_place_max_fanout]\
    [-routability_use_grt]\
    [-routability_target_rc_metric routability_target_rc_metric]\
    [-routability_check_overflow routability_check_overflow]\
    [-routability_snapshot_overflow routability_snapshot_overflow]\
    [-routability_max_density routability_max_density]\
    [-routability_inflation_ratio_coef routability_inflation_ratio_coef]\
    [-routability_max_inflation_ratio routability_max_inflation_ratio]\
    [-routability_rc_coefficients routability_rc_coefficients]\
    [-keep_resize_below_overflow keep_resize_below_overflow]\
    [-timing_driven_net_reweight_overflow timing_driven_net_reweight_overflow]\
    [-timing_driven_net_weight_max timing_driven_net_weight_max]\
    [-timing_driven_nets_percentage timing_driven_nets_percentage]\
    [-pad_left pad_left]\
    [-pad_right pad_right]\
    [-disable_revert_if_diverge]\
    [-enable_routing_congestion]
}

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {-random_seed -bin_grid_count -density \
      -init_density_penalty -init_wirelength_coef \
      -min_phi_coef -max_phi_coef -overflow \
      -reference_hpwl \
      -initial_place_max_iter -initial_place_max_fanout \
      -routability_check_overflow -routability_snapshot_overflow \
      -routability_max_density \
      -routability_target_rc_metric \
      -routability_inflation_ratio_coef \
      -routability_max_inflation_ratio \
      -routability_rc_coefficients \
      -timing_driven_net_reweight_overflow \
      -timing_driven_net_weight_max \
      -timing_driven_nets_percentage \
      -keep_resize_below_overflow \
      -pad_left -pad_right} \
    flags {-skip_initial_place \
      -skip_nesterov_place \
      -timing_driven \
      -routability_driven \
      -routability_use_grt \
      -skip_io \
      -incremental \
      -disable_revert_if_diverge \
      -enable_routing_congestion}

  sta::check_argc_eq0 "global_placement" $args

  if { [info exists flags(-incremental)] } {
    gpl::replace_incremental_place_cmd [array get keys] [array get flags]
  } else {
    gpl::replace_initial_place_cmd [array get keys] [array get flags]

    if { ![info exists flags(-skip_nesterov_place)] } {
      gpl::replace_nesterov_place_cmd [array get keys] [array get flags]
    }
  }
  gpl::replace_reset_cmd
}

sta::define_cmd_args "cluster_flops" {\
    [-tray_weight tray_weight]\
    [-timing_weight timing_weight]\
    [-max_split_size max_split_size]\
    [-num_paths num_paths]\
}

proc cluster_flops { args } {
  sta::parse_key_args "cluster_flops" args \
    keys { -tray_weight -timing_weight -max_split_size -num_paths } \
    flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 113 "No design block found."
  }

  set tray_weight 32.0
  set timing_weight 0.1
  set max_split_size 500
  set num_paths 0

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

  gpl::replace_run_mbff_cmd $max_split_size $tray_weight $timing_weight $num_paths
}

proc global_placement_debug { args } {
  sta::parse_key_args "global_placement_debug" args \
    keys {-pause -update -inst -start_iter -images_path \
      -start_rudy -rudy_stride} \
    flags {-draw_bins -initial -generate_images} ;# checker off

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
    $inst $start_iter $start_rudy $rudy_stride $generate_images $images_path
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

sta::define_cmd_args "read_soft_placement_clusters" {\
    -file file\
    [-min_weight min_weight]\
    [-min_cluster_size min_cluster_size]\
    [-max_cluster_size max_cluster_size]\
    [-split_large_clusters]\
    [-best_effort]\
}

proc read_soft_placement_clusters { args } {
  sta::parse_key_args "read_soft_placement_clusters" args \
    keys {-file -min_weight -min_cluster_size -max_cluster_size} \
    flags {-split_large_clusters -best_effort}

  if { ![info exists keys(-file)] } {
    utl::error GPL 1620 "read_soft_placement_clusters requires -file."
  }
  set file $keys(-file)

  set min_weight 0.0
  if { [info exists keys(-min_weight)] } {
    set min_weight_str $keys(-min_weight)
    if { [catch { expr {double($min_weight_str)} } min_weight] } {
      utl::error GPL 1627 "-min_weight must be a number."
    }
    if { $min_weight < 0.0 } {
      utl::error GPL 1628 "-min_weight must be >= 0.0."
    }
  }

  set min_cluster_size 2
  if { [info exists keys(-min_cluster_size)] } {
    set min_cluster_size $keys(-min_cluster_size)
    sta::check_positive_integer "-min_cluster_size" $min_cluster_size
  }

  set max_cluster_size 50
  if { [info exists keys(-max_cluster_size)] } {
    set max_cluster_size $keys(-max_cluster_size)
    sta::check_positive_integer "-max_cluster_size" $max_cluster_size
  }
  if { $max_cluster_size < $min_cluster_size } {
    utl::error GPL 1621 "-max_cluster_size must be >= -min_cluster_size."
  }

  set split_large_clusters [info exists flags(-split_large_clusters)]
  set best_effort [info exists flags(-best_effort)]

  set inst_best_cluster [dict create]
  set inst_best_weight [dict create]

  set fh [open $file r]
  set line_no 0
  while { [gets $fh line] >= 0 } {
    incr line_no
    set line [string trim $line]
    if { $line == "" } {
      continue
    }
    if { [string match "#*" $line] } {
      continue
    }

    set fields [split $line ","]
    if { [llength $fields] < 2 } {
      set msg "$file:$line_no: expected at least 2 CSV fields (inst_name,cluster_id[,weight]);"
      utl::warn GPL 1622 "$msg skipping."
      continue
    }

    set inst_name [string trim [lindex $fields 0]]
    set cluster_id [string trim [lindex $fields 1]]
    if { $inst_name == "" || $cluster_id == "" } {
      utl::warn GPL 1623 "$file:$line_no: empty inst_name/cluster_id; skipping."
      continue
    }

    if { $line_no == 1 && ($inst_name == "inst_name" \
        || $inst_name == "instance" || $inst_name == "inst") } {
      continue
    }

    set weight 1.0
    if { [llength $fields] >= 3 } {
      set weight_str [string trim [lindex $fields 2]]
      if { $weight_str != "" } {
        if { [catch { expr {double($weight_str)} } weight] } {
          utl::warn GPL 1624 "$file:$line_no: invalid weight '$weight_str'; skipping."
          continue
        }
      }
    }

    if { [dict exists $inst_best_weight $inst_name] } {
      if { $weight <= [dict get $inst_best_weight $inst_name] } {
        continue
      }
    }

    dict set inst_best_cluster $inst_name $cluster_id
    dict set inst_best_weight $inst_name $weight
  }
  close $fh

  set clusters [dict create]
  foreach inst_name [dict keys $inst_best_cluster] {
    set cluster_id [dict get $inst_best_cluster $inst_name]
    set weight [dict get $inst_best_weight $inst_name]
    if { $weight < $min_weight } {
      continue
    }
    dict lappend clusters $cluster_id $inst_name
  }

  set num_clusters_created 0
  foreach cluster_id [lsort [dict keys $clusters]] {
    set inst_names [dict get $clusters $cluster_id]
    set inst_names [lsort $inst_names]
    if { [llength $inst_names] < $min_cluster_size } {
      continue
    }

    if { [llength $inst_names] > $max_cluster_size } {
      if { !$split_large_clusters } {
        set msg "Cluster '$cluster_id' has [llength $inst_names] instances;"
        utl::warn GPL 1625 "$msg splitting into chunks of $max_cluster_size."
      }

      for { set i 0 } { $i < [llength $inst_names] } { incr i $max_cluster_size } {
        set chunk [lrange $inst_names $i [expr {$i + $max_cluster_size - 1}]]
        set insts []
        foreach name $chunk {
          if { [catch {
            set parsed [gpl::parse_inst_names read_soft_placement_clusters $name]
          } err] } {
            if { $best_effort } {
              continue
            }
            error $err
          }
          lappend insts {*}$parsed
        }
        if { [llength $insts] < $min_cluster_size } {
          continue
        }
        gpl::placement_cluster_cmd $insts
        incr num_clusters_created
      }
    } else {
      set insts []
      foreach name $inst_names {
        if { [catch {
          set parsed [gpl::parse_inst_names read_soft_placement_clusters $name]
        } err] } {
          if { $best_effort } {
            continue
          }
          error $err
        }
        lappend insts {*}$parsed
      }
      if { [llength $insts] < $min_cluster_size } {
        continue
      }
      gpl::placement_cluster_cmd $insts
      incr num_clusters_created
    }
  }

  utl::info GPL 1626 "Created $num_clusters_created placement clusters from $file."
}

namespace eval gpl {
proc get_global_placement_uniform_density { args } {
  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 114 "No design block found."
  }

  sta::parse_key_args "get_global_placement_uniform_density" args \
    keys { -pad_left -pad_right } \
    flags {} ;# checker off

  set uniform_density 0
  if { [ord::db_has_core_rows] } {
    sta::check_argc_eq0 "get_global_placement_uniform_density" $args

    set uniform_density [gpl::get_global_placement_uniform_density_cmd \
      [array get keys] [array get flags]]
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

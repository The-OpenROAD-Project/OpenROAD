# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

sta::define_cmd_args "rtl_macro_placer" { -max_num_macro  max_num_macro \
                                          -min_num_macro  min_num_macro \
                                          -max_num_inst   max_num_inst  \
                                          -min_num_inst   min_num_inst  \
                                          -tolerance      tolerance     \
                                          -max_num_level  max_num_level \
                                          -coarsening_ratio coarsening_ratio \
                                          -large_net_threshold large_net_threshold \
                                          -halo_width halo_width \
                                          -halo_height halo_height \
                                          -fence_lx   fence_lx \
                                          -fence_ly   fence_ly \
                                          -fence_ux   fence_ux \
                                          -fence_uy   fence_uy \
                                          -area_weight area_weight \
                                          -outline_weight outline_weight \
                                          -wirelength_weight wirelength_weight \
                                          -guidance_weight guidance_weight \
                                          -fence_weight fence_weight \
                                          -boundary_weight boundary_weight \
                                          -notch_weight notch_weight \
                                          -macro_blockage_weight macro_blockage_weight \
                                          -target_util   target_util \
                                          -min_ar  min_ar \
                                          -report_directory report_directory \
                                          -write_macro_placement file_name \
                                          -keep_clustering_data \
                                        }
proc rtl_macro_placer { args } {
  sta::parse_key_args "rtl_macro_placer" args \
    keys {-max_num_macro  -min_num_macro -max_num_inst  -min_num_inst  -tolerance   \
         -max_num_level  -coarsening_ratio -large_net_threshold \
         -halo_width -halo_height \
         -fence_lx   -fence_ly  -fence_ux   -fence_uy  \
         -area_weight  -outline_weight -wirelength_weight -guidance_weight -fence_weight \
         -boundary_weight -notch_weight \
         -macro_blockage_weight -target_util \
         -min_ar \
         -report_directory \
         -write_macro_placement } \
    flags {-keep_clustering_data}

  sta::check_argc_eq0 "rtl_macro_placer" $args

  #
  # Check for valid design
  if { [ord::get_db_block] == "NULL" } {
    utl::error MPL 1 "No block found for Macro Placement."
  }

  # Set the default parameters for the macro_placer
  # Set auto defaults for min/max std cells and macros based on design
  set max_num_macro 0
  set min_num_macro 0
  set max_num_inst 0
  set min_num_inst 0
  set tolerance 0.1
  set max_num_level 2
  set coarsening_ratio 10.0
  set large_net_threshold 50
  set halo_width 0.0
  set halo_height 0.0
  set fence_lx 0.0
  set fence_ly 0.0
  set fence_ux 0.0
  set fence_uy 0.0

  set area_weight 0.1
  set outline_weight 100.0
  set wirelength_weight 100.0
  set guidance_weight 10.0
  set fence_weight 10.0
  set boundary_weight 50.0
  set notch_weight 50.0
  set macro_blockage_weight 10.0
  set target_util 0.25
  set min_ar 0.33
  set report_directory "hier_rtlmp"

  if { [info exists keys(-max_num_macro)] } {
    set max_num_macro $keys(-max_num_macro)
  }
  if { [info exists keys(-min_num_macro)] } {
    set min_num_macro $keys(-min_num_macro)
  }
  if { [info exists keys(-max_num_inst)] } {
    set max_num_inst $keys(-max_num_inst)
  }
  if { [info exists keys(-min_num_inst)] } {
    set min_num_inst $keys(-min_num_inst)
  }

  if { [info exists keys(-tolerance)] } {
    set tolerance $keys(-tolerance)
  }

  if { [info exists keys(-max_num_level)] } {
    set max_num_level $keys(-max_num_level)
  }
  if { [info exists keys(-coarsening_ratio)] } {
    set coarsening_ratio $keys(-coarsening_ratio)
  }
  if { [info exists keys(-large_net_threshold)] } {
    set large_net_threshold $keys(-large_net_threshold)
  }

  if { [info exists keys(-halo_width)] && [info exists keys(-halo_height)] } {
    set halo_width $keys(-halo_width)
    set halo_height $keys(-halo_height)
  } elseif { [info exists keys(-halo_width)] } {
    set halo_width $keys(-halo_width)
    set halo_height $keys(-halo_width)
  } elseif { [info exists keys(-halo_height)] } {
    set halo_width $keys(-halo_height)
    set halo_height $keys(-halo_height)
  }

  if { [info exists keys(-fence_lx)] } {
    set fence_lx $keys(-fence_lx)
  }
  if { [info exists keys(-fence_ly)] } {
    set fence_ly $keys(-fence_ly)
  }
  if { [info exists keys(-fence_ux)] } {
    set fence_ux $keys(-fence_ux)
  }
  if { [info exists keys(-fence_uy)] } {
    set fence_uy $keys(-fence_uy)
  }
  if { [info exists keys(-area_weight)] } {
    set area_weight $keys(-area_weight)
  }
  if { [info exists keys(-wirelength_weight)] } {
    set wirelength_weight $keys(-wirelength_weight)
  }
  if { [info exists keys(-outline_weight)] } {
    set outline_weight $keys(-outline_weight)
  }
  if { [info exists keys(-guidance_weight)] } {
    set guidance_weight $keys(-guidance_weight)
  }
  if { [info exists keys(-fence_weight)] } {
    set fence_weight $keys(-fence_weight)
  }
  if { [info exists keys(-boundary_weight)] } {
    set boundary_weight $keys(-boundary_weight)
  }
  if { [info exists keys(-notch_weight)] } {
    set notch_weight $keys(-notch_weight)
  }
  if { [info exists keys(-macro_blockage_weight)] } {
    set macro_blockage_weight $keys(-macro_blockage_weight)
  }
  if { [info exists keys(-target_util)] } {
    set target_util $keys(-target_util)
  }
  if { [info exists keys(-min_ar)] } {
    set min_ar $keys(-min_ar)
  }
  if { [info exists keys(-report_directory)] } {
    set report_directory $keys(-report_directory)
  }

  file mkdir $report_directory

  if { [info exists keys(-write_macro_placement)] } {
    mpl::set_macro_placement_file $keys(-write_macro_placement)
  }

  if {
    ![mpl::rtl_macro_placer_cmd $max_num_macro \
      $min_num_macro \
      $max_num_inst \
      $min_num_inst \
      $tolerance \
      $max_num_level \
      $coarsening_ratio \
      $large_net_threshold \
      $halo_width \
      $halo_height \
      $fence_lx $fence_ly $fence_ux $fence_uy \
      $area_weight $outline_weight $wirelength_weight \
      $guidance_weight $fence_weight $boundary_weight \
      $notch_weight $macro_blockage_weight \
      $target_util \
      $min_ar \
      $report_directory \
      [info exists flags(-keep_clustering_data)]]
  } {
    return false
  }

  return true
}

sta::define_cmd_args "place_macro" {-macro_name macro_name \
                                    -location location \
                                    [-orientation orientation] \
                                    [-exact] \
                                    [-allow_overlap]
}

proc place_macro { args } {
  sta::parse_key_args "place_macro" args \
    keys {-macro_name -location -orientation} \
    flags {-exact -allow_overlap}

  if { [info exists keys(-macro_name)] } {
    set macro_name $keys(-macro_name)
  } else {
    utl::error MPL 19 "-macro_name is required."
  }

  set macro [mpl::parse_macro_name "place_macro" $macro_name]

  if { [info exists keys(-location)] } {
    set location $keys(-location)
  } else {
    utl::error MPL 22 "-location is required."
  }

  if { [llength $location] != 2 } {
    utl::error MPL 12 "-location is not a list of 2 values."
  }
  lassign $location x_origin y_origin
  set x_origin $x_origin
  set y_origin $y_origin

  set orientation R0
  if { [info exists keys(-orientation)] } {
    set orientation $keys(-orientation)
  }

  set exact [info exists flags(-exact)]
  set allow_overlap [info exists flags(-allow_overlap)]

  mpl::place_macro $macro $x_origin $y_origin $orientation $exact $allow_overlap
}

sta::define_cmd_args "set_macro_guidance_region" { -macro_name macro_name \
                                                   -region region }
proc set_macro_guidance_region { args } {
  sta::parse_key_args "set_macro_guidance_region" args \
    keys { -macro_name -region } flags {}

  sta::check_argc_eq0 "set_macro_guidance_region" $args

  if { [info exists keys(-macro_name)] } {
    set macro_name $keys(-macro_name)
  } else {
    utl::error MPL 43 "-macro_name is required."
  }

  set macro [mpl::parse_macro_name "set_macro_guidance_region" $macro_name]

  if { [info exists keys(-region)] } {
    set region $keys(-region)
  } else {
    utl::error MPL 30 "-region is required."
  }

  if { [llength $region] != 4 } {
    utl::error MPL 31 "-region must be a list of 4 values."
  }

  lassign $region x1 y1 x2 y2
  set x1 $x1
  set y1 $y1
  set x2 $x2
  set y2 $y2
  if { $x1 > $x2 } {
    utl::error MPL 32 "Invalid region: x1 > x2."
  } elseif { $y1 > $y2 } {
    utl::error MPL 33 "Invalid region: y1 > y2."
  }

  mpl::add_guidance_region $macro $x1 $y1 $x2 $y2
}

sta::define_cmd_args "set_macro_halo" { -macro_name macro_name \
                                        -halo halo }
proc set_macro_halo { args } {
  sta::parse_key_args "set_macro_halo" args \
    keys { -macro_name -halo } flags {}

  sta::check_argc_eq0 "set_macro_halo" $args

  if { [info exists keys(-macro_name)] } {
    set macro_name $keys(-macro_name)
  } else {
    utl::error MPL 48 "-macro_name is required."
  }

  set macro [mpl::parse_macro_name "set_macro_halo" $macro_name]

  if { [info exists keys(-halo)] } {
    set halo $keys(-halo)
  } else {
    utl::error MPL 38 "-halo is required."
  }

  if { [llength $halo] != 2 } {
    utl::error MPL 54 "-halo must be a list of 2 values."
  }

  lassign $halo width height
  set width $width
  set height $height

  mpl::set_macro_halo $macro $width $height
}

namespace eval mpl {
proc parse_macro_name { cmd macro_name } {
  set block [ord::get_db_block]
  set inst [$block findInst "$macro_name"]

  if { $inst == "NULL" } {
    utl::error MPL 20 "Couldn't find a macro named $macro_name."
  } elseif { ![$inst isBlock] } {
    utl::error MPL 21 "[$inst getName] is not a macro."
  }

  return $inst
}

proc mpl_debug { args } {
  sta::parse_key_args "mpl_debug" args \
    keys { -target_cluster_id target_cluster_id } \
    flags {-coarse -fine -show_bundled_nets \
           -show_clusters_ids \
           -skip_steps -only_final_result} ;# checker off

  set coarse [info exists flags(-coarse)]
  set fine [info exists flags(-fine)]
  if { !$coarse && !$fine } {
    set coarse true
    set fine true
  }
  set block [ord::get_db_block]

  set target_cluster_id -1
  if { [info exists keys(-target_cluster_id)] } {
    set target_cluster_id $keys(-target_cluster_id)
  }

  mpl::set_debug_cmd $block \
    $coarse \
    $fine \
    [info exists flags(-show_bundled_nets)] \
    [info exists flags(-show_clusters_ids)] \
    [info exists flags(-skip_steps)] \
    [info exists flags(-only_final_result)] \
    $target_cluster_id
}
}

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
                                          -min_channel_size min_channel_size \
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
                                          -soft_blockage_weight soft_blockage_weight \
                                          -target_util   target_util \
                                          -min_ar  min_ar \
                                          -report_directory report_directory \
                                          -write_macro_placement file_name \
                                          -keep_clustering_data \
                                          -pin_aware_channels \
                                        }
proc rtl_macro_placer { args } {
  sta::parse_key_args "rtl_macro_placer" args \
    keys {-max_num_macro -min_num_macro -max_num_inst -min_num_inst -tolerance \
         -max_num_level -coarsening_ratio -large_net_threshold \
         -min_channel_size \
         -halo_width -halo_height \
         -fence_lx -fence_ly -fence_ux -fence_uy \
         -area_weight -outline_weight -wirelength_weight -guidance_weight -fence_weight \
         -boundary_weight -notch_weight \
         -macro_blockage_weight \
         -soft_blockage_weight -target_util \
         -min_ar \
         -report_directory \
         -write_macro_placement } \
    flags {-keep_clustering_data -pin_aware_channels}

  sta::check_argc_eq0 "rtl_macro_placer" $args

  # Check for valid design
  if { [ord::get_db_block] == "NULL" } {
    utl::error MPL 1 "No block found for Macro Placement."
  }

  if { [info exists keys(-halo_width)] || [info exists keys(-halo_height)] } {
    utl::warn MPL 74 "-halo_width/-halo_height are deprecated, use\
                      -min_channel_size instead."
    set halo_width 0.0
    set halo_height 0.0

    if { [info exists keys(-halo_width)] } {
      set halo_width $keys(-halo_width)
      set halo_height $halo_width
    }

    if { [info exists keys(-halo_height)] } {
      set halo_height $keys(-halo_height)
      if { ![info exists keys(-halo_width)] } {
        set halo_width $halo_height
      }
    }

    mpl::set_min_channel [expr { 2 * $halo_width }] [expr { 2 * $halo_height }]
  }

  if { [info exists keys(-min_channel_size)] } {
    set min_channel_size $keys(-min_channel_size)
    set length [llength $min_channel_size]

    if { $length != 1 && $length != 2 } {
      utl::error MPL 80 "-min_channel_size must have 1 or 2 values."
    }

    if { $length == 1 } {
      set min_channel_width [lindex $min_channel_size 0]
      set min_channel_height $min_channel_width
    } else {
      lassign $min_channel_size min_channel_width min_channel_height
    }

    mpl::set_min_channel $min_channel_width $min_channel_height
  }

  if {
    [info exists flags(-pin_aware_channels)]
    && ![info exists keys(-min_channel_size)]
  } {
    utl::error MPL 79 "-pin_aware_channels requires -min_channel_size to be set."
  }

  if { [info exists keys(-macro_blockage_weight)] } {
    if { [info exists keys(-soft_blockage_weight)] } {
      utl::error MPL 69 "Cannot set -macro_blockage_weight along with\
                         -soft_blockage_weight. Use only one of those keys."
    }

    utl::warn MPL 70 "-macro_blockage_weight is deprecated, use\
                      -soft_blockage_weight instead."
    set keys(-soft_blockage_weight) $keys(-macro_blockage_weight)
  }

  if { ![mpl::rtl_macro_placer_cmd [array get keys] [array get flags]] } {
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

sta::define_cmd_args "set_macro_base_halo" { halo }
proc set_macro_base_halo { args } {
  utl::warn MPL 75 "set_macro_base_halo is deprecated, use\
                  -min_channel_size instead."
  sta::parse_key_args "set_macro_base_halo" args \
    keys {} flags {}

  lassign [mpl::parse_halo $args] left bottom right top
  set width [expr { $left + $right }]
  set height [expr { $bottom + $top }]

  mpl::set_min_channel $width $height
}

sta::define_cmd_args "block_macro_channels" {}
proc block_macro_channels { args } {
  sta::parse_key_args "block_macro_channels" args \
    keys {} flags {}

  sta::check_argc_eq0 "block_macro_channels" $args

  if { [ord::get_db_block] == "NULL" } {
    utl::error MPL 77 "Could not block macro channels. No block found."
  }

  mpl::block_macro_channels
}

namespace eval mpl {
proc parse_halo { halo } {
  set length [llength $halo]

  if { $length != 2 && $length != 4 } {
    utl::error MPL 72 "Halo must have 2 or 4 values."
  }

  if { $length == 2 } {
    lassign $halo left bottom
    set right $left
    set top $bottom
  }

  if { $length == 4 } {
    lassign $halo left bottom right top
  }

  foreach value [list $left $bottom $right $top] {
    if { $value < 0 } {
      utl::error MPL 73 "Halo values must be non-negative."
    }
  }

  return [list $left $bottom $right $top]
}

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
    keys { -target_cluster_id } \
    flags {-coarse -fine -show_bundled_nets \
           -show_clusters_ids \
           -skip_steps -only_final_result} ;# checker off

  mpl::set_debug_cmd [array get keys] [array get flags]
}
}

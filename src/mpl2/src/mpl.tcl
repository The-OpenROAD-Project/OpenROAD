############################################################################
## BSD 3-Clause License
##
## Copyright (c) 2021, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
############################################################################

sta::define_cmd_args "rtl_macro_placer" { -max_num_macro  max_num_macro \
                                          -min_num_macro  min_num_macro \
                                          -max_num_inst   max_num_inst  \
                                          -min_num_inst   min_num_inst  \
                                          -tolerance      tolerance     \
                                          -max_num_level  max_num_level \
                                          -coarsening_ratio coarsening_ratio \
                                          -num_bundled_ios  num_bundled_ios  \
                                          -large_net_threshold large_net_threshold \
                                          -signature_net_threshold signature_net_threshold \
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
                                          -pin_access_th pin_access_th \
                                          -target_util   target_util \
                                          -target_dead_space target_dead_space \
                                          -min_ar  min_ar \
                                          -snap_layer snap_layer \
                                          -bus_planning \
                                          -report_directory report_directory \
                                          -write_macro_placement file_name \
                                        }
proc rtl_macro_placer { args } {
  sta::parse_key_args "rtl_macro_placer" args \
    keys {-max_num_macro  -min_num_macro -max_num_inst  -min_num_inst  -tolerance   \
         -max_num_level  -coarsening_ratio  -num_bundled_ios  -large_net_threshold \
         -signature_net_threshold -halo_width -halo_height \
         -fence_lx   -fence_ly  -fence_ux   -fence_uy  \
         -area_weight  -outline_weight -wirelength_weight -guidance_weight -fence_weight \
         -boundary_weight -notch_weight -macro_blockage_weight  \
         -pin_access_th -target_util \
         -target_dead_space -min_ar -snap_layer \
         -report_directory \
         -write_macro_placement } \
    flags {-bus_planning}
  #
  # Check for valid design
  if {  [ord::get_db_block] == "NULL" } {
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
  set num_bundled_ios 3
  set large_net_threshold 50
  set signature_net_threshold 50
  set halo_width 0.0
  set halo_height 0.0
  set fence_lx 0.0
  set fence_ly 0.0
  set fence_ux 100000000.0
  set fence_uy 100000000.0

  set area_weight 0.1
  set outline_weight 100.0
  set wirelength_weight 100.0
  set guidance_weight 10.0
  set fence_weight 10.0
  set boundary_weight 50.0
  set notch_weight 10.0
  set macro_blockage_weight 10.0
  set pin_access_th 0.00
  set target_util 0.25
  set target_dead_space 0.05
  set min_ar 0.33
  set snap_layer -1
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
  if { [info exists keys(-num_bundled_ios)] } {
    set num_bundled_ios $keys(-num_bundled_ios)
  }
  if { [info exists keys(-large_net_threshold)] } {
    set large_net_threshold $keys(-large_net_threshold)
  }
  if { [info exists keys(-signature_net_threshold)] } {
    set signature_net_threshold $keys(-signature_net_threshold)
  }

  if { [info exists keys(-halo_width)] && [info exists keys(-halo_height)] } {
    set halo_width $keys(-halo_width)
    set halo_height $keys(-halo_height)
  } elseif {[info exists keys(-halo_width)]} {
    set halo_width $keys(-halo_width)
    set halo_height $keys(-halo_width)
  } elseif {[info exists keys(-halo_height)]} {
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
  if { [info exists keys(-pin_access_th)] } {
    set pin_access_th $keys(-pin_access_th)
  }
  if { [info exists keys(-target_util)] } {
    set target_util $keys(-target_util)
  }
  if { [info exists keys(-target_dead_space)] } {
    set target_dead_space $keys(-target_dead_space)
  }
  if { [info exists keys(-min_ar)] } {
    set min_ar $keys(-min_ar)
  }
  if { [info exists keys(-snap_layer)] } {
    set snap_layer $keys(-snap_layer)
  }
  if { [info exists keys(-report_directory)] } {
    set report_directory $keys(-report_directory)
  }

  file mkdir $report_directory

  if { [info exists keys(-write_macro_placement)] } {
    mpl2::set_macro_placement_file $keys(-write_macro_placement)
  }

  if {![mpl2::rtl_macro_placer_cmd $max_num_macro \
                                   $min_num_macro \
                                   $max_num_inst \
                                   $min_num_inst \
                                   $tolerance \
                                   $max_num_level \
                                   $coarsening_ratio \
                                   $num_bundled_ios \
                                   $large_net_threshold \
                                   $signature_net_threshold \
                                   $halo_width \
                                   $halo_height \
                                   $fence_lx $fence_ly $fence_ux $fence_uy \
                                   $area_weight $outline_weight $wirelength_weight \
                                   $guidance_weight $fence_weight $boundary_weight \
                                   $notch_weight $macro_blockage_weight \
                                   $pin_access_th \
                                   $target_util \
                                   $target_dead_space \
                                   $min_ar \
                                   $snap_layer \
                                   [info exists flags(-bus_planning)] \
                                   $report_directory \
                                   ]} {

    return false
  }

  return true
}

sta::define_cmd_args "place_macro" {-macro_name macro_name \
                                    -location location \
                                    [-orientation orientation] \
}

proc place_macro { args } {
  sta::parse_key_args "place_macro" args \
    keys {-macro_name -location -orientation} flags {}

  if {[info exists keys(-macro_name)]} {
    set macro_name $keys(-macro_name)
  } else {
    utl::error MPL 19 "-macro_name is required."
  }

  set macro [mpl2::parse_macro_name "place_macro" $macro_name]

  if {[info exists keys(-location)]} {
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
  if {[info exists keys(-orientation)]} {
    set orientation $keys(-orientation)
  }

  mpl2::place_macro $macro $x_origin $y_origin $orientation
}

namespace eval mpl2 {

proc parse_macro_name {cmd macro_name} {
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
    keys {} \
    flags {-coarse -fine -show_bundled_nets \
           -skip_steps -only_final_result};# checker off

  set coarse [info exists flags(-coarse)]
  set fine [info exists flags(-fine)]
  if { [expr !$coarse && !$fine] } {
    set coarse true
    set fine true
  }
  set block [ord::get_db_block]

  mpl2::set_debug_cmd $block \
    $coarse \
    $fine \
    [info exists flags(-show_bundled_nets)] \
    [info exists flags(-skip_steps)] \
    [info exists flags(-only_final_result)]
}

}

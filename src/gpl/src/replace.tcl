###############################################################################
## BSD 3-Clause License
##
## Copyright (c) 2018-2020, The Regents of the University of California
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
##   and#or other materials provided with the distribution.
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
###############################################################################

sta::define_cmd_args "global_placement" {\
    [-skip_initial_place]\
    [-skip_nesterov_place]\
    [-timing_driven]\
    [-routability_driven]\
    [-disable_timing_driven]\
    [-disable_routability_driven]\
    [-incremental]\
    [-force_cpu]\
    [-skip_io]\
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
    [-routability_max_density routability_max_density]\
    [-routability_max_bloat_iter routability_max_bloat_iter]\
    [-routability_max_inflation_iter routability_max_inflation_iter]\
    [-routability_inflation_ratio_coef routability_inflation_ratio_coef]\
    [-routability_max_inflation_ratio routability_max_inflation_ratio]\
    [-routability_rc_coefficients routability_rc_coefficients]\
    [-timing_driven_net_reweight_overflow timing_driven_net_reweight_overflow]\
    [-timing_driven_net_weight_max timing_driven_net_weight_max]\
    [-timing_driven_nets_percentage timing_driven_nets_percentage]\
    [-pad_left pad_left]\
    [-pad_right pad_right]\
}

proc global_placement { args } {
  sta::parse_key_args "global_placement" args \
    keys {-bin_grid_count -density \
      -init_density_penalty -init_wirelength_coef \
      -min_phi_coef -max_phi_coef -overflow \
      -reference_hpwl \
      -initial_place_max_iter -initial_place_max_fanout \
      -routability_check_overflow -routability_max_density \
      -routability_max_bloat_iter -routability_max_inflation_iter \
      -routability_target_rc_metric \
      -routability_inflation_ratio_coef \
      -routability_max_inflation_ratio \
      -routability_rc_coefficients \
      -timing_driven_net_reweight_overflow \
      -timing_driven_net_weight_max \
      -timing_driven_nets_percentage \
      -pad_left -pad_right} \
    flags {-skip_initial_place \
      -skip_nesterov_place \
      -timing_driven \
      -routability_driven \
      -routability_use_grt \
      -disable_timing_driven \
      -disable_routability_driven \
      -skip_io \
      -incremental\
      -force_cpu}

  # flow control for initial_place
  if { [info exists flags(-skip_initial_place)] } {
    gpl::set_initial_place_max_iter_cmd 0
  } elseif { [info exists keys(-initial_place_max_iter)] } {
    set initial_place_max_iter $keys(-initial_place_max_iter)
    sta::check_positive_integer "-initial_place_max_iter" $initial_place_max_iter
    gpl::set_initial_place_max_iter_cmd $initial_place_max_iter
  }

  set force_cpu [info exists flags(-force_cpu)]
  gpl::set_force_cpu $force_cpu

  set skip_io [info exists flags(-skip_io)]
  gpl::set_skip_io_mode_cmd $skip_io
  if { $skip_io } {
    gpl::set_initial_place_max_iter_cmd 0
  }

  set timing_driven [info exists flags(-timing_driven)]
  gpl::set_timing_driven_mode $timing_driven
  if { $timing_driven } {
    if { [get_libs -quiet "*"] == {} } {
      utl::error GPL 121 "No liberty libraries found."
    }

    if { $skip_io } {
      utl::warn "GPL" 150 "-skip_io will disable timing driven mode."
      gpl::set_timing_driven_mode 0
    }

    if { [info exists keys(-timing_driven_net_reweight_overflow)] } {
      set overflow_list $keys(-timing_driven_net_reweight_overflow)
    } else {
      set overflow_list [list 79 64 49 29 21 15]
    }

    foreach overflow $overflow_list {
      gpl::add_timing_net_reweight_overflow_cmd $overflow
    }

    if { [info exists keys(-timing_driven_net_weight_max)] } {
      gpl::set_timing_driven_net_weight_max_cmd $keys(-timing_driven_net_weight_max)
    }

    if { [info exists keys(-timing_driven_nets_percentage)] } {
      rsz::set_worst_slack_nets_percent $keys(-timing_driven_nets_percentage)
    }
  }

  if { [info exists flags(-disable_timing_driven)] } {
    utl::warn "GPL" 115 "-disable_timing_driven is deprecated."
  }

  set routability_driven [info exists flags(-routability_driven)]
  gpl::set_routability_driven_mode $routability_driven
  if { $routability_driven } {
    if { $skip_io } {
      utl::warn "GPL" 151 "-skip_io will disable routability driven mode."
      gpl::set_routability_driven_mode 0
    }
  }
  if { [info exists flags(-disable_routability_driven)] } {
    utl::warn "GPL" 116 "-disable_routability_driven is deprecated."
  }

  set routability_use_grt [info exists flags(-routability_use_grt)]
  gpl::set_routability_use_grt $routability_use_grt
  if { $routability_driven } {
    if { $routability_use_grt } {
      utl::warn "GPL" 152 \
        "Using GRT FastRoute instead of default RUDY for congestion in routability driven."
    }
  }

  if { [info exists keys(-initial_place_max_fanout)] } {
    set initial_place_max_fanout $keys(-initial_place_max_fanout)
    sta::check_positive_integer "-initial_place_max_fanout" $initial_place_max_fanout
    gpl::set_initial_place_max_fanout_cmd $initial_place_max_fanout
  }

  # density settings
  set target_density 0.7
  set uniform_mode 0

  if { [info exists keys(-density)] } {
    set target_density $keys(-density)
  }

  if { $target_density == "uniform" } {
    set uniform_mode 1
  } else {
    sta::check_positive_float "-density" $target_density
    if {$target_density > 1.0} {
      utl::error GPL 135 "Target density must be in \[0, 1\]."
    }
    gpl::set_density_cmd $target_density
  }

  gpl::set_uniform_target_density_mode_cmd $uniform_mode

  if { [info exists keys(-routability_max_density)] } {
    set routability_max_density $keys(-routability_max_density)
    sta::check_positive_float "-routability_max_density" $routability_max_density
    gpl::set_routability_max_density_cmd $routability_max_density
  }


  # parameter to control the RePlAce divergence
  if { [info exists keys(-min_phi_coef)] } {
    set min_phi_coef $keys(-min_phi_coef)
    sta::check_positive_float "-min_phi_coef" $min_phi_coef
    gpl::set_min_phi_coef_cmd $min_phi_coef
  }

  if { [info exists keys(-max_phi_coef)] } {
    set max_phi_coef $keys(-max_phi_coef)
    sta::check_positive_float "-max_phi_coef" $max_phi_coef
    gpl::set_max_phi_coef_cmd $max_phi_coef
  }

  if { [info exists keys(-init_density_penalty)] } {
    set density_penalty $keys(-init_density_penalty)
    sta::check_positive_float "-init_density_penalty" $density_penalty
    gpl::set_init_density_penalty_factor_cmd $density_penalty
  }

  if { [info exists keys(-init_wirelength_coef)] } {
    set coef $keys(-init_wirelength_coef)
    sta::check_positive_float "-init_wirelength_coef" $coef
    gpl::set_init_wirelength_coef_cmd $coef
  }

  if { [info exists keys(-reference_hpwl)] } {
    set reference_hpwl $keys(-reference_hpwl)
    sta::check_positive_float "-reference_hpwl" $reference_hpwl
    gpl::set_reference_hpwl_cmd $reference_hpwl
  }


  if { [info exists keys(-bin_grid_count)] } {
    set bin_grid_count $keys(-bin_grid_count)
    sta::check_positive_integer "-bin_grid_count" $bin_grid_count
    gpl::set_bin_grid_cnt_cmd $bin_grid_count $bin_grid_count
  }

  # overflow
  if { [info exists keys(-overflow)] } {
    set overflow $keys(-overflow)
    sta::check_positive_float "-overflow" $overflow
    gpl::set_overflow_cmd $overflow
  }

  # routability check overflow
  if { [info exists keys(-routability_check_overflow)] } {
    set routability_check_overflow $keys(-routability_check_overflow)
    sta::check_positive_float "-routability_check_overflow" $routability_check_overflow
    gpl::set_routability_check_overflow_cmd $routability_check_overflow
  }

  # routability bloat iter
  if { [info exists keys(-routability_max_bloat_iter)] } {
    set routability_max_bloat_iter $keys(-routability_max_bloat_iter)
    sta::check_positive_float "-routability_max_bloat_iter" $routability_max_bloat_iter
    gpl::set_routability_max_bloat_iter_cmd $routability_max_bloat_iter
  }

  # routability inflation iter
  if { [info exists keys(-routability_max_inflation_iter)] } {
    set routability_max_inflation_iter $keys(-routability_max_inflation_iter)
    sta::check_positive_float "-routability_max_inflation_iter" $routability_max_inflation_iter
    gpl::set_routability_max_inflation_iter_cmd $routability_max_inflation_iter
  }

  # routability inflation iter
  if { [info exists keys(-routability_target_rc_metric)] } {
    set target_rc_metric $keys(-routability_target_rc_metric)
    sta::check_positive_float "-routability_target_rc_metric" $target_rc_metric
    gpl::set_routability_target_rc_metric_cmd $target_rc_metric
  }

  # routability inflation ratio coef
  if { [info exists keys(-routability_inflation_ratio_coef)] } {
    set ratio_coef $keys(-routability_inflation_ratio_coef)
    sta::check_positive_float "-routability_inflation_ratio_coef" $ratio_coef
    gpl::set_routability_inflation_ratio_coef_cmd $ratio_coef
  }

  # routability max inflation ratio
  if { [info exists keys(-routability_max_inflation_ratio)] } {
    set max_inflation_ratio $keys(-routability_max_inflation_ratio)
    sta::check_positive_float "-routability_max_inflation_ratio" $max_inflation_ratio
    gpl::set_routability_max_inflation_ratio_cmd $max_inflation_ratio
  }

  # routability rc coefficients control
  if { [info exists keys(-routability_rc_coefficients)] } {
    set rc_coefficients $keys(-routability_rc_coefficients)
    set k1 [lindex $rc_coefficients 0]
    set k2 [lindex $rc_coefficients 1]
    set k3 [lindex $rc_coefficients 2]
    set k4 [lindex $rc_coefficients 3]
    gpl::set_routability_rc_coefficients_cmd $k1 $k2 $k3 $k4
  }

  # temp code.
  if { [info exists keys(-pad_left)] } {
    set pad_left $keys(-pad_left)
    sta::check_positive_integer "-pad_left" $pad_left
    gpl::set_pad_left_cmd $pad_left
  }
  if { [info exists keys(-pad_right)] } {
    set pad_right $keys(-pad_right)
    sta::check_positive_integer "-pad_right" $pad_right
    gpl::set_pad_right_cmd $pad_right
  }

  if { [ord::db_has_rows] } {
    sta::check_argc_eq0 "global_placement" $args

    if { [info exists flags(-incremental)] } {
      gpl::replace_incremental_place_cmd
    } else {
      gpl::replace_initial_place_cmd

      if { ![info exists flags(-skip_nesterov_place)] } {
        gpl::replace_nesterov_place_cmd
      }
    }
    gpl::replace_reset_cmd
  } else {
    utl::error GPL 130 "No rows defined in design. Use initialize_floorplan to add rows."
  }
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
    utl::error GPL 104 "No design block found."
  }

  set tray_weight 20.0
  set timing_weight 1.0
  set max_split_size 250
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
    keys {-pause -update -inst} \
    flags {-draw_bins -initial};# checker off

  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 105 "No design block found."
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

  set draw_bins [info exists flags(-draw_bins)]
  set initial [info exists flags(-initial)]

  gpl::set_debug_cmd $pause $update $draw_bins $initial $inst
}

namespace eval gpl {
proc get_global_placement_uniform_density { args } {

  if { [ord::get_db_block] == "NULL" } {
    utl::error GPL 106 "No design block found."
  }

  sta::parse_key_args "get_global_placement_uniform_density" args \
    keys { -pad_left -pad_right } \
    flags {};# checker off

  # no need for init IP, TD and RD
  gpl::set_initial_place_max_iter_cmd 0
  gpl::set_routability_driven_mode 0
  gpl::set_timing_driven_mode 0


  # pad setting
  if { [info exists keys(-pad_left)] } {
    set pad_left $keys(-pad_left)
    sta::check_positive_integer "-pad_left" $pad_left
    gpl::set_pad_left_cmd $pad_left
  }
  if { [info exists keys(-pad_right)] } {
    set pad_right $keys(-pad_right)
    sta::check_positive_integer "-pad_right" $pad_right
    gpl::set_pad_right_cmd $pad_right
  }

  set uniform_density 0
  if { [ord::db_has_rows] } {
    sta::check_argc_eq0 "get_global_placement_uniform_density" $args

    set uniform_density [gpl::get_global_placement_uniform_density_cmd]
    gpl::replace_reset_cmd

  } else {
    utl::error GPL 131 "No rows defined in design. Use initialize_floorplan to add rows."
  }
  return $uniform_density
}

}

#############################################################################
#
# BSD 3-Clause License
#
# Copyright (c) 2020, The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#############################################################################

sta::define_cmd_args "detailed_route" {
    [-output_maze filename]
    [-output_drc filename]
    [-output_cmap filename]
    [-output_guide_coverage filename]
    [-drc_report_iter_step step]
    [-db_process_node name]
    [-disable_via_gen]
    [-droute_end_iter iter]
    [-via_in_pin_bottom_layer layer]
    [-via_in_pin_top_layer layer]
    [-or_seed seed]
    [-or_k k]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-verbose level]
    [-distributed]
    [-remote_host rhost]
    [-remote_port rport]
    [-shared_volume vol]
    [-cloud_size sz]
    [-clean_patches]
    [-no_pin_access]
    [-min_access_points count]
    [-save_guide_updates]
    [-repair_pdn_vias layer]
    [-single_step_dr]
}

proc detailed_route { args } {
  sta::parse_key_args "detailed_route" args \
    keys {-output_maze -output_drc -output_cmap -output_guide_coverage \
      -db_process_node -droute_end_iter -via_in_pin_bottom_layer \
      -via_in_pin_top_layer -or_seed -or_k -bottom_routing_layer \
      -top_routing_layer -verbose -remote_host -remote_port -shared_volume \
      -cloud_size -min_access_points -repair_pdn_vias -drc_report_iter_step} \
    flags {-disable_via_gen -distributed -clean_patches -no_pin_access \
           -single_step_dr -save_guide_updates}
  sta::check_argc_eq0 "detailed_route" $args

  set enable_via_gen [expr ![info exists flags(-disable_via_gen)]]
  set clean_patches [expr [info exists flags(-clean_patches)]]
  set no_pin_access [expr [info exists flags(-no_pin_access)]]
  # single_step_dr is not a user option but is intended for algorithm
  # development.  It is not listed in the help string intentionally.
  set single_step_dr [expr [info exists flags(-single_step_dr)]]
  set save_guide_updates [expr [info exists flags(-save_guide_updates)]]

  if { [info exists keys(-repair_pdn_vias)] } {
    set repair_pdn_vias $keys(-repair_pdn_vias)
  } else {
    set repair_pdn_vias ""
  }
  if { [info exists keys(-output_maze)] } {
    set output_maze $keys(-output_maze)
  } else {
    set output_maze ""
  }
  if { [info exists keys(-output_drc)] } {
    set output_drc $keys(-output_drc)
  } else {
    set output_drc ""
  }
  if { [info exists keys(-drc_report_iter_step)] } {
    set drc_report_iter_step $keys(-drc_report_iter_step)
  } else {
    set drc_report_iter_step 0
  }
  if { [info exists keys(-output_cmap)] } {
    set output_cmap $keys(-output_cmap)
  } else {
    set output_cmap ""
  }
  if { [info exists keys(-output_guide_coverage)] } {
    set output_guide_coverage $keys(-output_guide_coverage)
  } else {
    set output_guide_coverage ""
  }
  if { [info exists keys(-db_process_node)] } {
    set db_process_node $keys(-db_process_node)
  } else {
    set db_process_node ""
  }
  if { [info exists keys(-droute_end_iter)] } {
    sta::check_positive_integer "-droute_end_iter" $keys(-droute_end_iter)
    if { $keys(-droute_end_iter) > 64 } {
      utl::warn "-droute_end_iter cannot be greater than 64. Setting -droute_end_iter to 64."
      set droute_end_iter 64
    } else {
      set droute_end_iter $keys(-droute_end_iter)
    }
  } else {
    set droute_end_iter -1
  }
  if { [info exists keys(-via_in_pin_bottom_layer)] } {
    set via_in_pin_bottom_layer $keys(-via_in_pin_bottom_layer)
  } else {
    set via_in_pin_bottom_layer ""
  }
  if { [info exists keys(-via_in_pin_top_layer)] } {
    set via_in_pin_top_layer $keys(-via_in_pin_top_layer)
  } else {
    set via_in_pin_top_layer ""
  }
  if { [info exists keys(-or_seed)] } {
    set or_seed $keys(-or_seed)
  } else {
    set or_seed -1
  }
  if { [info exists keys(-or_k)] } {
    set or_k $keys(-or_k)
  } else {
    set or_k 0
  }
  if { [info exists keys(-bottom_routing_layer)] } {
    set bottom_routing_layer $keys(-bottom_routing_layer)
  } else {
    set bottom_routing_layer ""
  }
  if { [info exists keys(-top_routing_layer)] } {
    set top_routing_layer $keys(-top_routing_layer)
  } else {
    set top_routing_layer ""
  }
  if { [info exists keys(-verbose)] } {
    sta::check_positive_integer "-verbose" $keys(-verbose)
    set verbose $keys(-verbose)
  } else {
    set verbose 1
  }
  if { [info exists flags(-distributed)] } {
    if { [info exists keys(-remote_host)] } {
      set rhost $keys(-remote_host)
    } else {
      utl::error DRT 506 "-remote_host is required for distributed routing."
    }
    if { [info exists keys(-remote_port)] } {
      set rport $keys(-remote_port)
    } else {
      utl::error DRT 507 "-remote_port is required for distributed routing."
    }
    if { [info exists keys(-shared_volume)] } {
      set vol $keys(-shared_volume)
    } else {
      utl::error DRT 508 "-shared_volume is required for distributed routing."
    }
    if { [info exists keys(-cloud_size)] } {
      set cloudsz $keys(-cloud_size)
    } else {
      utl::error DRT 516 "-cloud_size is required for distributed routing."
    }
    drt::detailed_route_distributed $rhost $rport $vol $cloudsz
  }
  if { [info exists keys(-min_access_points)] } {
    sta::check_cardinal "-min_access_points" $keys(-min_access_points)
    set min_access_points $keys(-min_access_points)
  } else {
    set min_access_points -1
  }
  drt::detailed_route_cmd $output_maze $output_drc $output_cmap \
    $output_guide_coverage $db_process_node $enable_via_gen $droute_end_iter \
    $via_in_pin_bottom_layer $via_in_pin_top_layer \
    $or_seed $or_k $bottom_routing_layer $top_routing_layer $verbose \
    $clean_patches $no_pin_access $single_step_dr $min_access_points \
    $save_guide_updates $repair_pdn_vias $drc_report_iter_step
}

proc detailed_route_num_drvs { args } {
  sta::check_argc_eq0 "detailed_route_num_drvs" $args
  return [drt::detailed_route_num_drvs]
}

sta::define_cmd_args "detailed_route_debug" {
    [-pa]
    [-ta]
    [-dr]
    [-maze]
    [-net name]
    [-pin name]
    [-box x1 y1 x2 y2]
    [-dump_last_worker]
    [-iter iter]
    [-pa_markers]
    [-dump_dr]
    [-dump_dir dir]
    [-pa_edge]
    [-pa_commit]
    [-write_net_tracks]
}

proc detailed_route_debug { args } {
  sta::parse_key_args "detailed_route_debug" args \
    keys {-net -iter -pin -dump_dir -box} \
    flags {-dr -maze -pa -pa_markers -pa_edge -pa_commit -dump_dr -ta \
           -write_net_tracks -dump_last_worker}

  sta::check_argc_eq0 "detailed_route_debug" $args

  set dr [info exists flags(-dr)]
  set dump_dr [info exists flags(-dump_dr)]
  set maze [info exists flags(-maze)]
  set pa [info exists flags(-pa)]
  set pa_markers [info exists flags(-pa_markers)]
  set pa_edge [info exists flags(-pa_edge)]
  set pa_commit [info exists flags(-pa_commit)]
  set ta [info exists flags(-ta)]
  set write_net_tracks [info exists flags(-write_net_tracks)]
  set dump_last_worker [info exists flags(-dump_last_worker)]

  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    set net_name ""
  }

  if { [info exists keys(-pin)] } {
    set pin_name $keys(-pin)
  } else {
    set pin_name ""
  }
  if { $dump_dr } {
    if { [info exists keys(-dump_dir)] } {
      set dump_dir $keys(-dump_dir)
    } else {
      utl::error DRT 2008 "-dump_dir is required for debugging with -dump_dr."
    }
  } else {
    set dump_dir ""
  }
  set box_x1 -1
  set box_y1 -1
  set box_x2 -1
  set box_y2 -1
  if { [info exists keys(-box)] } {
    set box $keys(-box)
    if { [llength $box] != 4 } {
      utl::error DRT 118 "-box is a list of 4 coordinates."
    }
    lassign $box box_x1 box_y1 box_x2 box_y2
    sta::check_positive_integer "-box" $box_x1
    sta::check_positive_integer "-box" $box_y1
  }

  if { [info exists keys(-iter)] } {
    set iter $keys(-iter)
  } else {
    set iter 0
  }

  drt::set_detailed_route_debug_cmd $net_name $pin_name $dr $dump_dr $pa $maze \
    $box_x1 $box_y1 $box_x2 $box_y2 $iter $pa_markers $pa_edge $pa_commit \
    $dump_dir $ta $write_net_tracks $dump_last_worker
}

sta::define_cmd_args "pin_access" {
    [-db_process_node name]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-min_access_points count]
    [-verbose level]
    [-distributed]
    [-remote_host rhost]
    [-remote_port rport]
    [-shared_volume vol]
    [-cloud_size sz]
}
proc pin_access { args } {
  sta::parse_key_args "pin_access" args \
    keys {-db_process_node -bottom_routing_layer -top_routing_layer -verbose \
          -min_access_points -remote_host -remote_port -shared_volume -cloud_size } \
    flags {-distributed}
  sta::check_argc_eq0 "detailed_route_debug" $args
  if { [info exists keys(-db_process_node)] } {
    set db_process_node $keys(-db_process_node)
  } else {
    set db_process_node ""
  }
  if { [info exists keys(-bottom_routing_layer)] } {
    set bottom_routing_layer $keys(-bottom_routing_layer)
  } else {
    set bottom_routing_layer ""
  }
  if { [info exists keys(-top_routing_layer)] } {
    set top_routing_layer $keys(-top_routing_layer)
  } else {
    set top_routing_layer ""
  }
  if { [info exists keys(-verbose)] } {
    sta::check_positive_integer "-verbose" $keys(-verbose)
    set verbose $keys(-verbose)
  } else {
    set verbose 1
  }
  if { [info exists keys(-min_access_points)] } {
    sta::check_cardinal "-min_access_points" $keys(-min_access_points)
    set min_access_points $keys(-min_access_points)
  } else {
    set min_access_points -1
  }
  if { [info exists flags(-distributed)] } {
    if { [info exists keys(-remote_host)] } {
      set rhost $keys(-remote_host)
    } else {
      utl::error DRT 552 "-remote_host is required for distributed routing."
    }
    if { [info exists keys(-remote_port)] } {
      set rport $keys(-remote_port)
    } else {
      utl::error DRT 553 "-remote_port is required for distributed routing."
    }
    if { [info exists keys(-shared_volume)] } {
      set vol $keys(-shared_volume)
    } else {
      utl::error DRT 554 "-shared_volume is required for distributed routing."
    }
    if { [info exists keys(-cloud_size)] } {
      set cloudsz $keys(-cloud_size)
    } else {
      utl::error DRT 555 "-cloud_size is required for distributed routing."
    }
    drt::detailed_route_distributed $rhost $rport $vol $cloudsz
  }
  drt::pin_access_cmd $db_process_node $bottom_routing_layer \
    $top_routing_layer $verbose $min_access_points
}

sta::define_cmd_args "detailed_route_run_worker" {
    [-dump_dir dir]
    [-worker_dir dir]
    [-drc_rpt drc]
} ;# checker off

proc detailed_route_run_worker { args } {
  sta::parse_key_args "detailed_route_run_worker" args \
    keys {-dump_dir -worker_dir -drc_rpt} \
    flags {} ;# checker off
  sta::check_argc_eq0 "detailed_route_run_worker" $args
  if { [info exists keys(-dump_dir)] } {
    set dump_dir $keys(-dump_dir)
  } else {
    utl::error DRT 517 "-dump_dir is required for detailed_route_run_worker command"
  }

  if { [info exists keys(-worker_dir)] } {
    set worker_dir $keys(-worker_dir)
  } else {
    utl::error DRT 520 "-worker_dir is required for detailed_route_run_worker command"
  }

  if { [info exists keys(-drc_rpt)] } {
    set drc_rpt $keys(-drc_rpt)
  } else {
    set drc_rpt ""
  }
  drt::run_worker_cmd $dump_dir $worker_dir $drc_rpt
}

sta::define_cmd_args "detailed_route_worker_debug" {
    [-maze_end_iter iter]
    [-drc_cost d_cost]
    [-marker_cost m_cost]
    [-fixed_shape_cost f_cost]
    [-marker_decay m_decay]
    [-ripup_mode mode]
    [-follow_guide f_guide]
} ;# checker off

proc detailed_route_worker_debug { args } {
  sta::parse_key_args "detailed_route_worker_debug" args \
    keys {-maze_end_iter -drc_cost -marker_cost -fixed_shape_cost \
          -marker_decay -ripup_mode -follow_guide} \
    flags {} ;# checker off
  if { [info exists keys(-maze_end_iter)] } {
    set maze_end_iter $keys(-maze_end_iter)
  } else {
    set maze_end_iter -1
  }

  if { [info exists keys(-drc_cost)] } {
    set drc_cost $keys(-drc_cost)
  } else {
    set drc_cost -1
  }

  if { [info exists keys(-marker_cost)] } {
    set marker_cost $keys(-marker_cost)
  } else {
    set marker_cost -1
  }

  if { [info exists keys(-fixed_shape_cost)] } {
    set fixed_shape_cost $keys(-fixed_shape_cost)
  } else {
    set fixed_shape_cost -1
  }

  if { [info exists keys(-marker_decay)] } {
    set marker_decay $keys(-marker_decay)
  } else {
    set marker_decay -1
  }

  if { [info exists keys(-ripup_mode)] } {
    set ripup_mode $keys(-ripup_mode)
  } else {
    set ripup_mode -1
  }

  if { [info exists keys(-follow_guide)] } {
    set follow_guide $keys(-follow_guide)
  } else {
    set follow_guide -1
  }
  drt::set_worker_debug_params $maze_end_iter $drc_cost $marker_cost \
    $fixed_shape_cost $marker_decay $ripup_mode $follow_guide
}

proc detailed_route_set_default_via { args } {
  sta::check_argc_eq1 "detailed_route_set_default_via" $args
  drt::detailed_route_set_default_via $args
}

proc detailed_route_set_unidirectional_layer { args } {
  sta::check_argc_eq1 "detailed_route_set_unidirectional_layer" $args
  drt::detailed_route_set_unidirectional_layer $args
}

namespace eval drt {
proc step_dr { args } {
  # args match FlexDR::SearchRepairArgs
  if { [llength $args] != 9 } {
    utl::error DRT 308 "step_dr requires nine positional arguments."
  }

  drt::detailed_route_step_drt {*}$args
}

sta::define_cmd_args "check_drc" {
    [-box box]
    [-output_file filename]
} ;# checker off
proc check_drc { args } {
  sta::parse_key_args "check_drc" args \
    keys { -box -output_file } \
    flags {} ;# checker off
  sta::check_argc_eq0 "check_drc" $args
  set box { 0 0 0 0 }
  if { [info exists keys(-box)] } {
    set box $keys(-box)
    if { [llength $box] != 4 } {
      utl::error DRT 612 "-box is a list of 4 coordinates."
    }
  }
  lassign $box x1 y1 x2 y2
  if { [info exists keys(-output_file)] } {
    set output_file $keys(-output_file)
  } else {
    utl::error DRT 613 "-output_file is required for check_drc command"
  }
  drt::check_drc_cmd $output_file $x1 $y1 $x2 $y2
}

proc fix_max_spacing { args } {
  sta::check_argc_eq0 "fix_max_spacing" $args
  drt::fix_max_spacing_cmd
}
}

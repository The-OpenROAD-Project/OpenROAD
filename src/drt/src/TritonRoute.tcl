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
    [-guide filename]
    [-output_guide filename]
    [-output_maze filename]
    [-output_drc filename]
    [-output_cmap filename]
    [-db_process_node name]
    [-disable_via_gen]
    [-droute_end_iter iter]
    [-via_in_pin_bottom_layer layer]
    [-via_in_pin_top_layer layer]
    [-or_seed seed]
    [-or_k_ k]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-verbose level]
    [-param filename]
    [-distributed]
    [-remote_host host]
    [-remote_port port]
    [-shared_volume vol]
}

proc detailed_route { args } {
  sta::parse_key_args "detailed_route" args \
    keys {-param -guide -output_guide -output_maze -output_drc -output_cmap \
      -db_process_node -droute_end_iter -via_in_pin_bottom_layer \
      -via_in_pin_top_layer -or_seed -or_k -bottom_routing_layer \
      -top_routing_layer -verbose -remote_host -remote_port -shared_volume} \
    flags {-disable_via_gen -distributed}
  sta::check_argc_eq0 "detailed_route" $args

  set enable_via_gen [expr ![info exists flags(-disable_via_gen)]]

  if { [info exists keys(-param)] } {
    if { [array size keys] > 1 } {
      utl::error DRT 251 "-param cannot be used with other arguments"
    } else {
      drt::detailed_route_cmd $keys(-param)
    }
  } else {
    if { [info exists keys(-guide)] } {
      set guide $keys(-guide)
    } else {
      set guide ""
    }
    if { [info exists keys(-output_guide)] } {
      set output_guide $keys(-output_guide)
    } else {
      set output_guide ""
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
    if { [info exists keys(-output_cmap)] } {
      set output_cmap $keys(-output_cmap)
    } else {
      set output_cmap ""
    }
    if { [info exists keys(-db_process_node)] } {
      set db_process_node $keys(-db_process_node)
    } else {
      set db_process_node ""
    }
    if { [info exists keys(-droute_end_iter)] } {
      sta::check_positive_integer "-droute_end_iter" $keys(-droute_end_iter)
      if {$keys(-droute_end_iter) > 64} {
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
        set host $keys(-remote_host)
      } else {
        utl::error DRT 506 "-remote_host is required for distributed routing."
      }
      if { [info exists keys(-remote_port)] } {
        set port $keys(-remote_port)
      } else {
        utl::error DRT 507 "-remote_port is required for distributed routing."
      }
      if { [info exists keys(-shared_volume)] } {
        set vol $keys(-shared_volume)
      } else {
        utl::error DRT 508 "-shared_volume is required for distributed routing."
      }
      drt::detailed_route_distributed $host $port $vol
    }
    drt::detailed_route_cmd $guide $output_guide $output_maze $output_drc \
      $output_cmap $db_process_node $enable_via_gen $droute_end_iter \
      $via_in_pin_bottom_layer $via_in_pin_top_layer \
      $or_seed $or_k $bottom_routing_layer $top_routing_layer $verbose
  }
}

proc detailed_route_num_drvs { args } {
  sta::check_argc_eq0 "detailed_route_num_drvs" $args
  return [drt::detailed_route_num_drvs]
}

sta::define_cmd_args "detailed_route_debug" {
    [-pa]
    [-dr]
    [-maze]
    [-net name]
    [-pin name]
    [-worker x y]
    [-iter iter]
    [-pa_markers]
    [-dump_dr]
    [-pa_edge]
    [-pa_commit]
}

proc detailed_route_debug { args } {
  sta::parse_key_args "detailed_route_debug" args \
      keys {-net -worker -iter -pin} \
      flags {-dr -maze -pa -pa_markers -pa_edge -pa_commit -dump_dr}

  sta::check_argc_eq0 "detailed_route_debug" $args

  set dr [info exists flags(-dr)]
  set dump_dr [info exists flags(-dump_dr)]
  set maze [info exists flags(-maze)]
  set pa [info exists flags(-pa)]
  set pa_markers [info exists flags(-pa_markers)]
  set pa_edge [info exists flags(-pa_edge)]
  set pa_commit [info exists flags(-pa_commit)]

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

  set worker_x -1
  set worker_y -1
  if [info exists keys(-worker)] {
    set worker $keys(-worker)
    if { [llength $worker] != 2 } {
      utl::error DRT 118 "-worker is a list of 2 coordinates."
    }
    lassign $worker worker_x worker_y
    sta::check_positive_integer "-worker" $worker_x
    sta::check_positive_integer "-worker" $worker_y
  }

  if { [info exists keys(-iter)] } {
    set iter $keys(-iter)
  } else {
    set iter 0
  }

  drt::set_detailed_route_debug_cmd $net_name $pin_name $dr $dump_dr $pa $maze \
      $worker_x $worker_y $iter $pa_markers $pa_edge $pa_commit
}
sta::define_cmd_args "pin_access" {
    [-db_process_node name]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-verbose level]
}
proc pin_access { args } {
  sta::parse_key_args "pin_access" args \
      keys {-db_process_node -bottom_routing_layer -top_routing_layer -verbose} \
      flags {}
  sta::check_argc_eq0 "detailed_route_debug" $args
  if [info exists keys(-db_process_node)] {
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
  drt::pin_access_cmd $db_process_node $bottom_routing_layer $top_routing_layer $verbose
}
proc detailed_route_run_worker { args } {
  sta::check_argc_eq1 "detailed_route_run_worker" $args
  drt::run_worker_cmd $args
}

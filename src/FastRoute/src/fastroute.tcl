###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
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
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

sta::define_cmd_args "set_global_routing_layer_adjustment" { [-layer layer] \
                                                             [-adjustment adj] \
}

proc set_global_routing_layer_adjustment { args } {
  sta::parse_key_args "set_global_routing_layer_adjustment" args \
    keys {-layer -adjustment} \

  set layer -1
  if { [info exists keys(-layer)] } {
    set layer $keys(-layer)
  } else {
    ord::error "\[Global routing layer adjustment\] Missing layer"
  }

  set adj -0.1
  if { [info exists keys(-adjustment)] } {
    set adj $keys(-adjustment)
  } else {
    ord::error "\[Global routing layer adjustment\] Missing adjustment"
  }

  sta::check_positive_integer "-layer" $layer
  sta::check_positive_float "-adjustment" $adj

  FastRoute::add_layer_adjustment $layer $adj
}

sta::define_cmd_args "set_pdrev_topology_priority" { [-net net] \
                                                     [-alpha alpha] \
}

proc set_pdrev_topology_priority { args } {
  sta::parse_key_args "set_pdrev_topology_priority" args \
    keys {-net -alpha} \

  set net "INVALID"
  if { [info exists keys(-net)] } {
    set net $keys(-net)
  } else {
    ord::error "\[PDRev topology priority\] Missing net name"
  }

  set alpha -0.1
  if { [info exists keys(-alpha)] } {
    set alpha $keys(-alpha)
  } else {
    ord::error "\[PDRev topology priority\] Missing alpha"
  }

  sta::check_positive_float "-alpha" $alpha
  FastRoute::set_alpha_for_net $net $alpha
}

sta::define_cmd_args "set_global_routing_layer_pitch" { [-layer layer] \
                                                        [-pitch pitch] \
}

proc set_global_routing_layer_pitch { args } {
  sta::parse_key_args "set_global_routing_layer_pitch" args \
    keys {-layer -pitch} \

  set layer -1
  if { [info exists keys(-layer)] } {
    set layer $keys(-layer)
  } else {
    ord::error "\[Global routing layer pitch\] Missing layer"
  }

  set pitch -0.1
  if { [info exists keys(-pitch)] } {
    set pitch $keys(-pitch)
  } else {
    ord::error "\[Global routing layer pitch\] Missing pitch"
  }

  sta::check_positive_integer "-layer" $layer
  sta::check_positive_float "-pitch" $pitch
  
  FastRoute::set_layer_pitch $layer $pitch
}

sta::define_cmd_args "set_clock_route_flow" { [-min_layer min_layer] \
                                              [-max_layer max_layer] \
}

proc set_clock_route_flow { args } {
  sta::parse_key_args "set_clock_route_flow" args \
    keys {-min_layer -max_layer} \

  set min_layer -1
  if { [info exists keys(-min_layer)] } {
    set min_layer $keys(-min_layer)
  } else {
    ord::error "\[Clock route flow\] Missing min_layer"
  }

  set max_layer -1
  if { [info exists keys(-max_layer)] } {
    set max_layer $keys(-max_layer)
  } else {
    ord::error "\[Clock route flow\] Missing max_layer"
  }

  sta::check_positive_integer "min_layer" $min_layer
  sta::check_positive_integer "max_layer" $max_layer

  if { $min_layer < $max_layer } {
    FastRoute::set_layer_range_for_clock $min_layer $max_layer
  } else {
    ord::error "Min routing layer is greater than max routing layer"
  }
}

sta::define_cmd_args "repair_antenna" { [-diode_cell_name cell_name] \
                                        [-diode_pin_name pin_name] \
}

proc repair_antenna { args } {
  sta::parse_key_args "fastroute" args \
    keys {-diode_cell_name -diode_pin_name} \

  set cell_name "INVALID"
  if { [info exists keys(-diode_cell_name)] } {
    set cell_name $keys(-diode_cell_name)
  } else {
    ord::error "Missing antenna cell name"
  }

  set pin_name "INVALID"
  if { [info exists keys(-diode_pin_name)] } {
    set pin_name $keys(-diode_pin_name)
  } else {
    ord::error "Missing antenna cell pin name"
  }

  FastRoute::repair_antenna $cell_name $pin_name
}

sta::define_cmd_args "write_guides" { file_name }


proc write_guides { args } {
  set file_name $args
  FastRoute::write_guides $file_name
}

sta::define_cmd_args "fastroute" {[-output_file out_file] \
                                           [-capacity_adjustment cap_adjust] \
                                           [-min_routing_layer min_layer] \
                                           [-max_routing_layer max_layer] \
                                           [-unidirectional_routing] \
                                           [-tile_size tile_size] \
                                           [-layers_adjustments layers_adjustments] \
                                           [-regions_adjustments regions_adjustments] \
                                           [-alpha alpha] \
                                           [-verbose verbose] \
                                           [-overflow_iterations iterations] \
                                           [-grid_origin origin] \
                                           [-pdrev_for_high_fanout fanout] \
                                           [-allow_overflow] \
                                           [-seed seed] \
                                           [-report_congestion congest_file] \
                                           [-layers_pitches layers_pitches] \
}

proc fastroute { args } {
  sta::parse_key_args "fastroute" args \
    keys {-output_file -capacity_adjustment -min_routing_layer -max_routing_layer \
          -tile_size -alpha -verbose -layers_adjustments \
          -regions_adjustments -overflow_iterations \
          -grid_origin -pdrev_for_high_fanout -seed -report_congestion -layers_pitches} \
    flags {-unidirectional_routing -allow_overflow} \

  if { [info exists keys(-capacity_adjustment)] } {
    set cap_adjust $keys(-capacity_adjustment)
    sta::check_positive_float "-capacity_adjustment" $cap_adjust
    FastRoute::set_capacity_adjustment $cap_adjust
  } else {
    FastRoute::set_capacity_adjustment 0.0
  }

  if { [info exists keys(-min_routing_layer)] } {
    set min_layer $keys(-min_routing_layer)
    sta::check_positive_integer "-min_routing_layer" $min_layer
    FastRoute::set_min_layer $min_layer
  } else {
    FastRoute::set_min_layer 1
  }

  set max_layer -1
  if { [info exists keys(-max_routing_layer)] } {
    set max_layer $keys(-max_routing_layer)
    sta::check_positive_integer "-max_routing_layer" $max_layer
    FastRoute::set_max_layer $max_layer
  } else {
    FastRoute::set_max_layer -1
  }

  if { [info exists keys(-tile_size)] } {
    set tile_size $keys(-tile_size)
    FastRoute::set_tile_size $tile_size
  }

  if { [info exists keys(-layers_adjustments)] } {
    ord::warn "option -layers_adjustments is deprecated. use command \
    set_global_routing_layer_adjustment <layer> <adjustment>"
    set layers_adjustments $keys(-layers_adjustments)
    foreach layer_adjustment $layers_adjustments {
      if { [llength $layer_adjustment] == 2 } {
        lassign $layer_adjustment layer reductionPercentage
        FastRoute::add_layer_adjustment $layer $reductionPercentage
      } else {
        ord::error "Wrong number of arguments for layer adjustments"
      }
    }
  }
  
  if { [info exists keys(-regions_adjustments)] } {
    set regions_adjustments $keys(-regions_adjustments)
    foreach region_adjustment $regions_adjustments {
      if { [llength $region_adjustment] == 2 } {
        lassign $region_adjustment minX minY maxX maxY layer reductionPercentage
        puts "Adjust region ($minX, $minY); ($maxX, $maxY) in layer $layer \
          in [expr $reductionPercentage * 100]%"
        FastRoute::add_region_adjustment $minX $minY $maxX $maxY $layer $reductionPercentage
      } else {
        ord::error "Wrong number of arguments for region adjustments"
      }
    }
  }

  FastRoute::set_unidirectional_routing [info exists flags(-unidirectional_routing)]

  if { [info exists keys(-alpha) ] } {
    set alpha $keys(-alpha)
    sta::check_positive_float "-alpha" $alpha
    FastRoute::set_alpha $alpha
  } else {
    FastRoute::set_alpha 0.3
  }

  if { [info exists keys(-verbose) ] } {
    set verbose $keys(-verbose)
    FastRoute::set_verbose $verbose
  } else {
    FastRoute::set_verbose 0
  }
  
  if { [info exists keys(-overflow_iterations) ] } {
    set iterations $keys(-overflow_iterations)
    sta::check_positive_integer "-overflow_iterations" $iterations
    FastRoute::set_overflow_iterations $iterations
  } else {
    FastRoute::set_overflow_iterations 50
  }

  if { [info exists keys(-grid_origin)] } {
    set origin $keys(-grid_origin)
    if { [llength $origin] == 2 } {
      lassign $origin origin_x origin_y
      FastRoute::set_grid_origin $origin_x $origin_y
    } else {
      ord::error "Wrong number of arguments for origin"
    }
  } else {
    FastRoute::set_grid_origin -1 -1
  }

  if { [info exists keys(-pdrev_for_high_fanout)] } {
    set fanout $keys(-pdrev_for_high_fanout)
    FastRoute::set_pdrev_for_high_fanout $fanout
  } else {
    FastRoute::set_pdrev_for_high_fanout -1
  }

  if { [info exists keys(-seed) ] } {
    set seed $keys(-seed)
    FastRoute::set_seed $seed
  } else {
    FastRoute::set_seed 0
  }

  FastRoute::set_allow_overflow [info exists flags(-allow_overflow)]

  if { [info exists keys(-report_congestion)] } {
    set congest_file $keys(-report_congestion)
    FastRoute::report_congestion $congest_file
  }

  if { [info exists keys(-layers_pitches)] } {
    ord::warn "option -layers_pitches is deprecated. use command \
    set_global_routing_layer_pitch <layer> <adjustment>"
    set layers_pitches $keys(-layers_pitches)
    foreach layer_pitch $layers_pitches {
      if { [llength $layer_pitch] == 2 } {
        lassign $layer_pitch layer pitch
        sta::check_positive_integer "-layer" $layer
        sta::check_positive_float "-pitch" $pitch
        FastRoute::set_layer_pitch $layer $pitch
      } else {
        ord::error "Wrong number of arguments for layer pitches"
      }
    }
  }

  if { ![ord::db_has_tech] } {
    ord::error "missing dbTech"
  }

  if { [ord::get_db_block] == "NULL" } {
    ord::error "missing dbBlock"
  }

  for {set layer 1} {$layer <= $max_layer} {set layer [expr $layer+1]} {
    if { !([ord::db_layer_has_hor_tracks $layer] && \
         [ord::db_layer_has_ver_tracks $layer]) } {
      ord::error "missing track structure"
    }
  }

  FastRoute::start_fastroute
  FastRoute::run_fastroute

  if { [info exists keys(-output_file)] } {
    ord::warn "option -output_file is deprecated. use command \
    write_guides <file_name>"
    set out_file $keys(-output_file)
    FastRoute::write_guides $out_file
  }
}

namespace eval FastRoute {

proc estimate_rc_cmd {} {
  if { [have_routes] } {
    estimate_rc
  } else {
    ord::error "run fastroute before estimating parasitics for global routing."
  }
}

# FastRoute namespace end
}


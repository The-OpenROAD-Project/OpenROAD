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


sta::define_cmd_args "fastroute" {[-output_file out_file] \
                                           [-capacity_adjustment cap_adjust] \
                                           [-min_routing_layer min_layer] \
                                           [-max_routing_layer max_layer] \
                                           [-unidirectional_routing] \
                                           [-tile_size tile_size] \
                                           [-layers_adjustments layers_adjustments] \
                                           [-regions_adjustments regions_adjustments] \
                                           [-nets_alphas_priorities nets_alphas] \
                                           [-alpha alpha] \
                                           [-verbose verbose] \
                                           [-overflow_iterations iterations] \
                                           [-max_routing_length max_length] \
                                           [-max_length_per_layer max_length_per_layer] \
                                           [-grid_origin origin] \
                                           [-pdrev_for_high_fanout fanout] \
                                           [-allow_overflow] \
                                           [-estimateRC] \
                                           [-seed seed] \
                                           [-report_congestion congest_file] \
                                           [-layers_pitches layers_pitches] \
                                           [-antenna_avoidance_flow] \
                                           [-antenna_cell_name antenna_cell_name] \
                                           [-antenna_pin_name antenna_pin_name] \
                                           [-clock_nets_route_flow] \
                                           [-min_layer_for_clock_net min_clock_layer] \
}

proc fastroute { args } {
  sta::parse_key_args "fastroute" args \
    keys {-output_file -capacity_adjustment -min_routing_layer -max_routing_layer \
          -tile_size -alpha -verbose -layers_adjustments \
          -regions_adjustments -nets_alphas_priorities -overflow_iterations \
          -grid_origin -pdrev_for_high_fanout -seed -report_congestion -layers_pitches \
          -max_routing_length -max_length_per_layer -min_layer_for_clock_net -antenna_cell_name -antenna_pin_name} \
    flags {-unidirectional_routing -allow_overflow -estimateRC -clock_nets_route_flow -antenna_avoidance_flow} \

  if { [info exists keys(-output_file)] } {
    set out_file $keys(-output_file)
    FastRoute::set_output_file $out_file
  } else {
    puts "\[WARNING\] Default output guide name: out.guide"
    FastRoute::set_output_file "out.guide"
  }

  if { [info exists keys(-capacity_adjustment)] } {
    set cap_adjust $keys(-capacity_adjustment)
    FastRoute::set_capacity_adjustment $cap_adjust
  } else {
    FastRoute::set_capacity_adjustment 0.0
  }

  if { [info exists keys(-min_routing_layer)] } {
    set min_layer $keys(-min_routing_layer)
    FastRoute::set_min_layer $min_layer
  } else {
    FastRoute::set_min_layer 1
  }

  set max_layer -1
  if { [info exists keys(-max_routing_layer)] } {
    set max_layer $keys(-max_routing_layer)
    FastRoute::set_max_layer $max_layer
  } else {
    FastRoute::set_max_layer -1
  }

  if { [info exists keys(-tile_size)] } {
    set tile_size $keys(-tile_size)
    FastRoute::set_tile_size $tile_size
  }

  if { [info exists keys(-layers_adjustments)] } {
    set layers_adjustments $keys(-layers_adjustments)
    foreach layer_adjustment $layers_adjustments {
      set layer [lindex $layer_adjustment 0]
      set reductionPercentage [lindex $layer_adjustment 1]

      FastRoute::add_layer_adjustment $layer $reductionPercentage
    }
  }
  
  if { [info exists keys(-regions_adjustments)] } {
    set regions_adjustments $keys(-regions_adjustments)
    foreach region_adjustment $regions_adjustments {
      set minX [lindex $region_adjustment 0]
      set minY [lindex $region_adjustment 1]
      set maxX [lindex $region_adjustment 2]
      set maxY [lindex $region_adjustment 3]
      set layer [lindex $region_adjustment 4]
      set reductionPercentage [lindex $region_adjustment 5]

      puts "Adjust region ($minX, $minY); ($maxX, $maxY) in layer $layer \
        in [expr $reductionPercentage * 100]%"
      FastRoute::add_region_adjustment $minX $minY $maxX $maxY $layer $reductionPercentage
    }
  }
  
  if { [info exists keys(-nets_alphas_priorities)] } {
    set nets_alphas $keys(-nets_alphas_priorities)
    foreach net_alpha $nets_alphas {
      set net_name [lindex $net_alpha 0]
      set alpha [lindex $net_alpha 1]

      FastRoute::set_alpha_for_net $net_name $alpha
    }
  }

  if { [info exists flags(-unidirectional_routing)] } {
    FastRoute::set_unidirectional_routing 1
  } else {
    FastRoute::set_unidirectional_routing 0
  }

  if { [info exists keys(-alpha) ] } {
    set alpha $keys(-alpha)
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
    FastRoute::set_overflow_iterations $iterations
  } else {
    FastRoute::set_overflow_iterations 50
  }

  if { [info exists keys(-max_routing_length)] } {
          set max_length $keys(-max_routing_length)
          FastRoute::set_max_routing_length $max_length
  }

  if { [info exists keys(-max_length_per_layer)] } {
    set max_length_per_layer $keys(-max_length_per_layer)
    foreach length_per_layer $max_length_per_layer {
      set layer [lindex $length_per_layer 0]
      set length [lindex $length_per_layer 1]

      puts "Max length in layer $layer:  $length um"
      FastRoute::add_layer_max_length $layer $length
    }
  }

  if { [info exists keys(-grid_origin)] } {
    set origin $keys(-grid_origin)

    set origin_x [lindex $origin 0]
    set origin_y [lindex $origin 1]

    FastRoute::set_grid_origin $origin_x $origin_y
  } else {
    FastRoute::set_grid_origin -1 -1
  }

  if { [info exists keys(-pdrev_for_high_fanout)] } {
    set faonut $keys(-pdrev_for_high_fanout)

    FastRoute::set_pdrev_for_high_fanout $faonut
  } else {
    FastRoute::set_pdrev_for_high_fanout -1
  }

  if { [info exists keys(-seed) ] } {
    set seed $keys(-seed)
    FastRoute::set_seed $seed
  } else {
    FastRoute::set_seed 0
  }

  if { [info exists flags(-allow_overflow)] } {
    FastRoute::set_allow_overflow 1
  } else {
    FastRoute::set_allow_overflow 0
  }

  if { [info exists keys(-report_congestion)] } {
    set congest_file $keys(-report_congestion)
    FastRoute::report_congestion $congest_file
  }

  if { [info exists keys(-layers_pitches)] } {
    set layers_pitches $keys(-layers_pitches)
    foreach layer_pitch $layers_pitches {
      set layer [lindex $layer_pitch 0]
      set pitch [lindex $layer_pitch 1]

      FastRoute::set_layer_pitch $layer $pitch
    }
  }

  if { [info exists flags(-antenna_avoidance_flow)] } {
    set diode_cell_name "INVALID"
    if { [info exists keys(-antenna_cell_name)] } {
      set diode_cell_name $keys(-antenna_cell_name)
    } else {
      ord::error "Missing antenna cell name"
    }

    set diode_pin_name "INVALID"
    if { [info exists keys(-antenna_pin_name)] } {
      set diode_pin_name $keys(-antenna_pin_name)
    } else {
      ord::error "Missing antenna cell pin name"
    }

    FastRoute::enable_antenna_avoidance_flow $diode_cell_name $diode_pin_name
  }

  if { [info exists flags(-clock_nets_route_flow)] } {
    FastRoute::set_clock_nets_route_flow 1
  } else {
    FastRoute::set_clock_nets_route_flow 0
  }

  set min_clock_layer 6
  if { [info exists keys(-min_layer_for_clock_net)] } {
    set min_clock_layer $keys(-min_layer_for_clock_net)
    FastRoute::set_min_layer_for_clock $min_clock_layer
  } elseif { [info exists flags(-clock_nets_route_flow)] } {
    puts "\[WARNING\] Using the default min layer for clock nets routing (layer $min_clock_layer)"
    FastRoute::set_min_layer_for_clock $min_clock_layer
  }

  for {set layer 1} {$layer <= $max_layer} {set layer [expr $layer+1]} {
    if { !([ord::db_layer_has_hor_tracks $layer] && \
         [ord::db_layer_has_ver_tracks $layer]) } {
      ord::error "missing track structure"
    }
  }

  if {[info exists flags(-estimateRC)]} {
    FastRoute::set_estimate_rc
  }
  FastRoute::start_fastroute
  if {[info exists flags(-estimateRC)]} {
    FastRoute::estimate_rc
  } else {
    FastRoute::run_fastroute
    FastRoute::write_guides
  }
}

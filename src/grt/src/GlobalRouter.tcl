###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
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

sta::define_cmd_args "set_global_routing_layer_adjustment" { layer adj }

proc set_global_routing_layer_adjustment { args } {
  if {[llength $args] == 2} {
    lassign $args layer adj

    if {$layer == "*"} {
      sta::check_positive_float "adjustment" $adj
      grt::set_capacity_adjustment $adj
    } elseif [regexp -all {([a-zA-Z0-9]+)-([a-zA-Z0-9]+)} $layer] {
      lassign [grt::parse_layer_range "set_global_routing_layer_adjustment" $layer] first_layer last_layer
      for {set l $first_layer} {$l <= $last_layer} {incr l} {
        grt::check_routing_layer $l
        sta::check_positive_float "adjustment" $adj

        grt::add_layer_adjustment $l $adj
      }
    } else {
      set layer_idx [grt::parse_layer_name $layer]
      grt::check_routing_layer $layer_idx
      sta::check_positive_float "adjustment" $adj

      grt::add_layer_adjustment $layer_idx $adj
    }
  } else {
    utl::error GRT 44 "set_global_routing_layer_adjustment: Wrong number of arguments."
  }
}

sta::define_cmd_args "set_routing_alpha" { alpha \
                                          [-net net_name] }

proc set_routing_alpha { args } {
  sta::parse_key_args "set_routing_alpha" args \
                 keys {-net}

  set alpha [lindex $args 0]
  if { ![string is double $alpha] || $alpha < 0.0 || $alpha > 1.0 } {
    utl::error GRT 29 "The alpha value must be between 0.0 and 1.0."
  }
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
    grt::set_alpha_for_net $net_name $alpha
  } elseif { [llength $args] == 1 } {
    grt::set_routing_alpha_cmd $alpha
  } else {
    utl::error GRT 46 "set_routing_alpha: Wrong number of arguments."
  }
}

sta::define_cmd_args "set_global_routing_region_adjustment" { region \
                                                              [-layer layer] \
                                                              [-adjustment adjustment] \
}

proc set_global_routing_region_adjustment { args } {
  sta::parse_key_args "set_global_routing_region_adjustment" args \
                 keys {-layer -adjustment}

  if { ![ord::db_has_tech] } {
    utl::error GRT 47 "missing dbTech."
  }
  set tech [ord::get_db_tech]
  set lef_units [$tech getLefUnits]

  if { [info exists keys(-layer)] } {
    set layer $keys(-layer)
  } else {
    utl::error GRT 48 "set_global_routing_region_adjustment: Missing layer."
  }

  if { [info exists keys(-adjustment)] } {
    set adjustment $keys(-adjustment)
  } else {
    utl::error GRT 49 "set_global_routing_region_adjustment: Missing adjustment."
  }

  sta::check_argc_eq1 "set_global_routing_region_adjustment" $args
  set region [lindex $args 0]
  if {[llength $region] == 4} {
    lassign $region lower_x lower_y upper_x upper_y
    sta::check_positive_float "lower_left_x" $lower_x
    sta::check_positive_float "lower_left_y" $lower_y
    sta::check_positive_float "upper_right_x" $upper_x
    sta::check_positive_float "upper_right_y" $upper_y
    sta::check_positive_integer "-layer" $layer
    sta::check_positive_float "-adjustment" $adjustment

    set lower_x [expr { int($lower_x * $lef_units) }]
    set lower_y [expr { int($lower_y * $lef_units) }]
    set upper_x [expr { int($upper_x * $lef_units) }]
    set upper_y [expr { int($upper_y * $lef_units) }]

    grt::check_region $lower_x $lower_y $upper_x $upper_y

    grt::add_region_adjustment $lower_x $lower_y $upper_x $upper_y $layer $adjustment
  } else {
    utl::error GRT 50 "set_global_routing_region_adjustment: Wrong number of arguments to define a region."
  }
}

sta::define_cmd_args "set_routing_layers" { [-signal layers] \
                                            [-clock layers] \
}

proc set_routing_layers { args } {
  sta::parse_key_args "set_routing_layers" args \
    keys {-signal -clock}

  if { [info exists keys(-signal)] } {
    grt::define_layer_range $keys(-signal)
  }

  if { [info exists keys(-clock)] } {
    grt::define_clock_layer_range $keys(-clock)
  }
}

sta::define_cmd_args "set_macro_extension" { extension }

proc set_macro_extension { args } {
  if {[llength $args] == 1} {
    lassign $args extension
    sta::check_positive_integer "macro_extension" $extension
    grt::set_macro_extension $extension
  } else {
    utl::error GRT 219 "set_macro_extension: Wrong number of arguments."
  }
}

sta::define_cmd_args "set_global_routing_random" { [-seed seed] \
                                                   [-capacities_perturbation_percentage percent] \
                                                   [-perturbation_amount value]
                                                 }

proc set_global_routing_random { args } {
  sta::parse_key_args "set_global_routing_random" args \
  keys { -seed -capacities_perturbation_percentage -perturbation_amount }

  if { [info exists keys(-seed)] } {
    set seed $keys(-seed)
    sta::check_integer "set_global_routing_random" $seed
    grt::set_seed $seed
  }

  if { [info exists keys(-capacities_perturbation_percentage)] } {
    set percentage $keys(-capacities_perturbation_percentage)
    sta::check_percent "set_global_routing_random" $percentage
    grt::set_capacities_perturbation_percentage $percentage
  }

  if { [info exists keys(-perturbation_amount)] } {
    set perturbation $keys(-perturbation_amount)
    sta::check_positive_integer "set_global_routing_random" $perturbation
    grt::set_perturbation_amount $perturbation
  }
}

sta::define_cmd_args "global_route" {[-guide_file out_file] \
                                  [-verbose verbose] \
                                  [-congestion_iterations iterations] \
                                  [-grid_origin origin] \
                                  [-allow_congestion] \
                                  [-overflow_iterations iterations] \
                                  [-allow_overflow]
}

proc global_route { args } {
  sta::parse_key_args "global_route" args \
    keys {-guide_file -verbose \ 
          -congestion_iterations \
          -overflow_iterations -grid_origin
         } \
    flags {-allow_congestion -allow_overflow}

  if { ![ord::db_has_tech] } {
    utl::error GRT 51 "missing dbTech."
  }

  if { [ord::get_db_block] == "NULL" } {
    utl::error GRT 52 "missing dbBlock."
  }

  if { [info exists keys(-verbose) ] } {
    set verbose $keys(-verbose)
    grt::set_verbose $verbose
  } else {
    grt::set_verbose 0
  }

  if { [info exists keys(-grid_origin)] } {
    set origin $keys(-grid_origin)
    if { [llength $origin] == 2 } {
      lassign $origin origin_x origin_y
      grt::set_grid_origin $origin_x $origin_y
    } else {
      utl::error GRT 55 "Wrong number of arguments for origin."
    }
  } else {
    grt::set_grid_origin 0 0
  }

  if { [info exists keys(-congestion_iterations) ] } {
    set iterations $keys(-congestion_iterations)
    sta::check_positive_integer "-congestion_iterations" $iterations
    grt::set_overflow_iterations $iterations
  } else {
    grt::set_overflow_iterations 50
  }

  if { [info exists keys(-overflow_iterations)] } {
    utl::war GRT 147 "-overflow_iterations is deprecated. Use -congestion_iterations."
    set iterations $keys(-overflow_iterations)
    sta::check_positive_integer "-overflow_iterations" $iterations
    grt::set_overflow_iterations $iterations
  }

  if { [info exists flags(-allow_overflow)] } {
    utl::warn GRT 146 "-allow_overflow is deprecated. Use -allow_congestion."
  }

  set allow_congestion [expr [info exists flags(-allow_congestion)] || [info exists flags(-allow_overflow)]]
  grt::set_allow_congestion $allow_congestion

  grt::clear
  grt::run

  if { [info exists keys(-guide_file)] } {
    set out_file $keys(-guide_file)
    grt::write_guides $out_file
  }
}

sta::define_cmd_args "repair_antennas" { lib_port \
                                         [-iterations iterations]}

proc repair_antennas { args } {
  sta::parse_key_args "repair_antennas" args \
                 keys {-iterations}
  if { [grt::have_routes] } {
    sta::check_argc_eq1 "repair_antennas" $args
    set lib_port [lindex $args 0]
    if { ![sta::is_object $lib_port] } {
      set lib_port [sta::get_lib_pins [lindex $args 0]]
    }

    if { [info exists keys(-iterations)] } {
      set iterations $keys(-iterations)
      sta::check_positive_integer "-repair_antennas_iterations" $iterations
    } else {
      set iterations 1
    }

    if { $lib_port != "" } {
      grt::repair_antennas $lib_port $iterations
    } else {
      utl::error GRT 69 "Diode not found."
    }
  } else {
    utl::error GRT 45 "Run global_route before repair_antennas."
  }
}

sta::define_cmd_args "write_guides" { file_name }

proc write_guides { args } {
  set file_name $args
  grt::write_guides $file_name
}

namespace eval grt {

proc estimate_rc_cmd {} {
  if { [have_routes] } {
    estimate_rc
  } else {
    utl::error GRT 58 "run global_route before estimating parasitics for global routing."
  }
}

proc check_routing_layer { layer } {
  if { ![ord::db_has_tech] } {
    utl::error GRT 59 "no technology has been read."
  }
  sta::check_positive_integer "layer" $layer

  set tech [ord::get_db_tech]
  set max_routing_layer [$tech getRoutingLayerCount]
  set tech_layer [$tech findRoutingLayer $layer]
  
  set min_tech_layer [$tech findRoutingLayer 1]
  set max_tech_layer [$tech findRoutingLayer $max_routing_layer]
  
  if {$layer > $max_routing_layer} {
    utl::error GRT 60 "layer [$tech_layer getConstName] is greater than the max routing layer ([$max_tech_layer getConstName])."
  }
  if {$layer < 1} {
    utl::error GRT 61 "layer [$tech_layer getConstName] is lesser than the min routing layer ([$min_tech_layer getConstName])."
  }
}

proc parse_layer_name { layer_name } {
  if { ![ord::db_has_tech] } {
    utl::error GRT 222 "no technology has been read."
  }
  set tech [ord::get_db_tech]
  set tech_layer [$tech findLayer $layer_name]
  if { $tech_layer == "NULL" } {
    utl::error GRT 5 "layer $layer_name not found."
  }
  set layer_idx [$tech_layer getRoutingLevel]

  return $layer_idx
}

proc parse_layer_range { cmd layer_range } {
  if [regexp -all {([a-zA-Z0-9]+)-([a-zA-Z0-9]+)} $layer_range - min_layer_name max_layer_name] {
    set min_layer [parse_layer_name $min_layer_name]
    set max_layer [parse_layer_name $max_layer_name]

    set layers "$min_layer $max_layer"
    return $layers
  } else {
    utl::error GRT 62 "Input format to define layer range for $cmd is min-max."
  }
}

proc check_region { lower_x lower_y upper_x upper_y } {
  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 63 "missing dbBlock."
  }

  set core_area [$block getDieArea]

  if {$lower_x < [$core_area xMin] || $lower_x > [$core_area xMax]} {
    utl::error GRT 64 "Lower left x is outside die area."
  }

  if {$lower_y < [$core_area yMin] || $lower_y > [$core_area yMax]} {
    utl::error GRT 65 "Lower left y is outside die area."
  }

  if {$upper_x < [$core_area xMin] || $upper_x > [$core_area xMax]} {
    utl::error GRT 66 "Upper right x is outside die area."
  }

  if {$upper_y < [$core_area yMin] || $upper_y > [$core_area yMax]} {
    utl::error GRT 67 "Upper right y is outside die area."
  }
}

proc highlight_route { net_name } {
  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GRT 223 "missing dbBlock."
  }
  set net [$block findNet $net_name]
  if { $net != "NULL" } {
    highlight_net_route $net
  }
}

proc define_layer_range { layers } {
  set layer_range [grt::parse_layer_range "-layers" $layers]
  lassign $layer_range min_layer max_layer
  grt::check_routing_layer $min_layer
  grt::check_routing_layer $max_layer

  grt::set_min_layer $min_layer
  grt::set_max_layer $max_layer

  for {set layer 1} {$layer <= $max_layer} {set layer [expr $layer+1]} {
    if { !([ord::db_layer_has_hor_tracks $layer] && \
         [ord::db_layer_has_ver_tracks $layer]) } {
      utl::error GRT 57 "missing track structure."
    }
  }
}

proc define_clock_layer_range { layers } {
  set layer_range [grt::parse_layer_range "-clock_layers" $layers]
  lassign $layer_range min_clock_layer max_clock_layer
  grt::check_routing_layer $min_clock_layer
  grt::check_routing_layer $max_clock_layer

  if { $min_clock_layer < $max_clock_layer } {
    grt::set_clock_layer_range $min_clock_layer $max_clock_layer
  } else {
    utl::error GRT 56 "-clock_layers: Min routing layer is greater than max routing layer."
  }
}

# grt namespace end
}

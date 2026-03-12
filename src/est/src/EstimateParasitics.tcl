# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

namespace eval est {
proc get_db_tech_checked { } {
  set tech [ord::get_db_tech]
  if { $tech == "NULL" } {
    utl::error EST 210 "No technology loaded."
  }
  return $tech
}

# namespace eval est
}

sta::define_cmd_args "estimate_parasitics" { -placement|-global_routing \
                                            [-spef_file filename]}

proc estimate_parasitics { args } {
  sta::parse_key_args "estimate_parasitics" args \
    keys {-spef_file} flags {-placement -global_routing}

  set filename ""
  if { [info exists keys(-spef_file)] } {
    set filename $keys(-spef_file)
  }

  if { [info exists flags(-placement)] } {
    if { [est::check_corner_wire_caps] } {
      est::estimate_parasitics_cmd "placement" $filename
    }
  } elseif { [info exists flags(-global_routing)] } {
    if { [grt::have_routes] } {
      # should check for layer rc
      est::estimate_parasitics_cmd "global_routing" $filename
    } else {
      utl::error EST 5 "Run global_route before estimating parasitics for global routing."
    }
  } else {
    utl::error EST 3 "missing -placement or -global_routing flag."
  }
}

# Units are from OpenSTA (ie Liberty file or set_cmd_units).
sta::define_cmd_args "set_layer_rc" { [-layer layer]\
                                        [-via via_layer]\
                                        [-capacitance cap]\
                                        [-resistance res]\
                                        [-corner corner]}
proc set_layer_rc { args } {
  sta::parse_key_args "set_layer_rc" args \
    keys {-layer -via -capacitance -resistance -corner} \
    flags {}

  if { [info exists keys(-layer)] && [info exists keys(-via)] } {
    utl::error "EST" 201 "Use -layer or -via but not both."
  }

  set corners [sta::parse_scene_or_null keys]
  set tech [est::get_db_tech_checked]
  if { [info exists keys(-layer)] } {
    set layer_name $keys(-layer)
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "EST" 202 "layer $layer_name not found."
    }

    if { [$layer getRoutingLevel] == 0 } {
      utl::error "EST" 203 "$layer_name is not a routing layer."
    }

    if { ![info exists keys(-capacitance)] && ![info exists keys(-resistance)] } {
      utl::error "EST" 204 "missing -capacitance or -resistance argument."
    }

    set cap 0.0
    if { [info exists keys(-capacitance)] } {
      set cap $keys(-capacitance)
      sta::check_positive_float "-capacitance" $cap
      # F/m
      set cap [expr [sta::capacitance_ui_sta $cap] / [sta::distance_ui_sta 1.0]]
    }

    set res 0.0
    if { [info exists keys(-resistance)] } {
      set res $keys(-resistance)
      sta::check_positive_float "-resistance" $res
      # ohm/m
      set res [expr [sta::resistance_ui_sta $res] / [sta::distance_ui_sta 1.0]]
    }

    if { $corners == "NULL" } {
      set corners [sta::scenes]
      # Only update the db layers if -corner not specified.
      est::set_dblayer_wire_rc $layer $res $cap
    }

    foreach corner $corners {
      est::set_layer_rc_cmd $layer $corner $res $cap
    }
  } elseif { [info exists keys(-via)] } {
    set layer_name $keys(-via)
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "EST" 205 "via $layer_name not found."
    }

    if { [info exists keys(-capacitance)] } {
      utl::warn "EST" 206 "-capacitance not supported for vias."
    }

    if { [info exists keys(-resistance)] } {
      set res $keys(-resistance)
      sta::check_positive_float "-resistance" $res
      set res [sta::resistance_ui_sta $res]

      if { $corners == "NULL" } {
        set corners [sta::scenes]
        # Only update the db layers if -corner not specified.
        est::set_dbvia_wire_r $layer $res
      }

      foreach corner $corners {
        est::set_layer_rc_cmd $layer $corner $res 0.0
      }
    } else {
      utl::error "EST" 208 "no -resistance specified for via."
    }
  } else {
    utl::error "EST" 209 "missing -layer or -via argument."
  }
}

sta::define_cmd_args "report_layer_rc" {[-corner corner]}

proc report_layer_rc { args } {
  sta::parse_key_args "report_layer_rc" args \
    keys {-corner} \
    flags {}
  set corner [sta::parse_scene_or_null keys]
  set tech [est::get_db_tech_checked]
  set no_routing_layers [$tech getRoutingLayerCount]
  ord::ensure_units_initialized
  set res_unit \
    "[sta::unit_scale_abbrev_suffix "resistance"]/[sta::unit_scale_abbrev_suffix "distance"]"
  set cap_unit \
    "[sta::unit_scale_abbrev_suffix "capacitance"]/[sta::unit_scale_abbrev_suffix "distance"]"
  set res_convert [expr [sta::resistance_sta_ui 1.0] / [sta::distance_sta_ui 1.0]]
  set cap_convert [expr [sta::capacitance_sta_ui 1.0] / [sta::distance_sta_ui 1.0]]

  puts "   Layer   | Unit Resistance | Unit Capacitance "
  puts [format "           | %15s | %16s" [format "(%s)" $res_unit] [format "(%s)" $cap_unit]]
  puts "------------------------------------------------"
  for { set i 1 } { $i <= $no_routing_layers } { incr i } {
    set layer [$tech findRoutingLayer $i]
    if { $corner == "NULL" } {
      lassign [est::dblayer_wire_rc $layer] layer_wire_res layer_wire_cap
    } else {
      set layer_wire_res [est::layer_resistance $layer $corner]
      set layer_wire_cap [est::layer_capacitance $layer $corner]
    }
    set res_ui [expr $layer_wire_res * $res_convert]
    set cap_ui [expr $layer_wire_cap * $cap_convert]
    puts [format "%10s | %15.2e | %16.2e" [$layer getName] $res_ui $cap_ui]
  }
  puts "------------------------------------------------"

  set res_unit "[sta::unit_scale_abbrev_suffix "resistance"]"
  set res_convert [sta::resistance_sta_ui 1.0]
  puts ""
  puts "   Layer   | Via Resistance "
  puts [format "           | %14s " [format "(%s)" $res_unit]]
  puts "----------------------------"
  # ignore the last routing layer (no via layer above it)
  for { set i 1 } { $i < $no_routing_layers } { incr i } {
    set layer [[$tech findRoutingLayer $i] getUpperLayer]
    if { $corner == "NULL" } {
      set layer_via_res [$layer getResistance]
    } else {
      set layer_via_res [est::layer_resistance $layer $corner]
    }
    set res_ui [expr $layer_via_res * $res_convert]
    puts [format "%10s | %14.2e " [$layer getName] $res_ui]
  }
  puts "----------------------------"
}

sta::define_cmd_args "set_wire_rc" {[-clock] [-signal] [-data]\
                                      [-layers layers]\
                                      [-layer layer]\
                                      [-h_resistance h_res]\
                                      [-h_capacitance h_cap]\
                                      [-v_resistance v_res]\
                                      [-v_capacitance v_cap]\
                                      [-resistance res]\
                                      [-capacitance cap]\
                                      [-corner corner]}

proc set_wire_rc { args } {
  sta::parse_key_args "set_wire_rc" args \
    keys {-layer -layers -resistance -capacitance -corner \
          -h_resistance -h_capacitance -v_resistance -v_capacitance} \
    flags {-clock -signal -data}

  set corner [sta::parse_scene_or_null keys]

  set h_wire_res 0.0
  set h_wire_cap 0.0
  set v_wire_res 0.0
  set v_wire_cap 0.0

  if { [info exists keys(-layers)] } {
    if {
      [info exists keys(-h_resistance)]
      || [info exists keys(-h_capacitance)]
      || [info exists keys(-v_resistance)]
      || [info exists keys(-v_capacitance)]
    } {
      utl::error EST 1 "Use -layers or -resistance/-capacitance but not both."
    }
    if { [info exists keys(-layer)] } {
      utl::error EST 6 "Use -layers or -layer but not both."
    }
    set total_h_wire_res 0.0
    set total_h_wire_cap 0.0
    set total_v_wire_res 0.0
    set total_v_wire_cap 0.0

    set h_layers 0
    set v_layers 0

    set layers $keys(-layers)

    foreach layer_name $layers {
      set tec_layer [[est::get_db_tech_checked] findLayer $layer_name]
      if { $tec_layer == "NULL" } {
        utl::error EST 2 "layer $layer_name not found."
      }
      if { $corner == "NULL" } {
        lassign [est::dblayer_wire_rc $tec_layer] layer_wire_res layer_wire_cap
      } else {
        set layer_wire_res [est::layer_resistance $tec_layer $corner]
        set layer_wire_cap [est::layer_capacitance $tec_layer $corner]
      }
      set layer_direction [$tec_layer getDirection]
      if { $layer_direction == "HORIZONTAL" } {
        set total_h_wire_res [expr { $total_h_wire_res + $layer_wire_res }]
        set total_h_wire_cap [expr { $total_h_wire_cap + $layer_wire_cap }]
        incr h_layers
      } elseif { $layer_direction == "VERTICAL" } {
        set total_v_wire_res [expr { $total_v_wire_res + $layer_wire_res }]
        set total_v_wire_cap [expr { $total_v_wire_cap + $layer_wire_cap }]
        incr v_layers
      } else {
        set total_h_wire_res [expr { $total_h_wire_res + $layer_wire_res }]
        set total_h_wire_cap [expr { $total_h_wire_cap + $layer_wire_cap }]
        incr h_layers
        set total_v_wire_res [expr { $total_v_wire_res + $layer_wire_res }]
        set total_v_wire_cap [expr { $total_v_wire_cap + $layer_wire_cap }]
        incr v_layers
      }

      if { [info exists flags(-clock)] } {
        est::add_clk_layer_cmd $tec_layer
      }

      if { [info exists flags(-signal)] } {
        est::add_signal_layer_cmd $tec_layer
      }

      if { ![info exists flags(-clock)] && ![info exists flags(-signal)] } {
        est::add_clk_layer_cmd $tec_layer
        est::add_signal_layer_cmd $tec_layer
      }
    }
    if { $h_layers == 0 } {
      utl::error EST 16 "No horizontal layer specified."
    }
    if { $v_layers == 0 } {
      utl::error EST 17 "No vertical layer specified."
    }

    set h_wire_res [expr $total_h_wire_res / $h_layers]
    set h_wire_cap [expr $total_h_wire_cap / $h_layers]
    set v_wire_res [expr $total_v_wire_res / $v_layers]
    set v_wire_cap [expr $total_v_wire_cap / $v_layers]
  } elseif { [info exists keys(-layer)] } {
    set layer_name $keys(-layer)
    set tec_layer [[est::get_db_tech_checked] findLayer $layer_name]
    if { $tec_layer == "NULL" } {
      utl::error EST 15 "layer $tec_layer not found."
    }

    if { $corner == "NULL" } {
      lassign [est::dblayer_wire_rc $tec_layer] h_wire_res h_wire_cap
      lassign [est::dblayer_wire_rc $tec_layer] v_wire_res v_wire_cap
    } else {
      set h_wire_res [est::layer_resistance $tec_layer $corner]
      set v_wire_res [est::layer_resistance $tec_layer $corner]
      set h_wire_cap [est::layer_capacitance $tec_layer $corner]
      set v_wire_cap [est::layer_capacitance $tec_layer $corner]
    }

    if { [info exists flags(-clock)] } {
      est::add_clk_layer_cmd $tec_layer
    }

    if { [info exists flags(-signal)] } {
      est::add_signal_layer_cmd $tec_layer
    }

    if { ![info exists flags(-clock)] && ![info exists flags(-signal)] } {
      est::add_clk_layer_cmd $tec_layer
      est::add_signal_layer_cmd $tec_layer
    }
  } else {
    ord::ensure_units_initialized
    if { [info exists keys(-resistance)] } {
      set res $keys(-resistance)
      sta::check_positive_float "-resistance" $res
      set h_wire_res [expr [sta::resistance_ui_sta $res] / [sta::distance_ui_sta 1.0]]
      set v_wire_res [expr [sta::resistance_ui_sta $res] / [sta::distance_ui_sta 1.0]]
    }

    if { [info exists keys(-capacitance)] } {
      set cap $keys(-capacitance)
      sta::check_positive_float "-capacitance" $cap
      set h_wire_cap [expr [sta::capacitance_ui_sta $cap] / [sta::distance_ui_sta 1.0]]
      set v_wire_cap [expr [sta::capacitance_ui_sta $cap] / [sta::distance_ui_sta 1.0]]
    }

    if { [info exists keys(-h_resistance)] } {
      set h_res $keys(-h_resistance)
      sta::check_positive_float "-h_resistance" $h_res
      set h_wire_res [expr [sta::resistance_ui_sta $h_res] / [sta::distance_ui_sta 1.0]]
    }

    if { [info exists keys(-h_capacitance)] } {
      set h_cap $keys(-h_capacitance)
      sta::check_positive_float "-h_capacitance" $h_cap
      set h_wire_cap [expr [sta::capacitance_ui_sta $h_cap] / [sta::distance_ui_sta 1.0]]
    }

    if { [info exists keys(-v_resistance)] } {
      set v_res $keys(-v_resistance)
      sta::check_positive_float "-v_resistance" $v_res
      set v_wire_res [expr [sta::resistance_ui_sta $v_res] / [sta::distance_ui_sta 1.0]]
    }

    if { [info exists keys(-v_capacitance)] } {
      set v_cap $keys(-v_capacitance)
      sta::check_positive_float "-v_capacitance" $v_cap
      set v_wire_cap [expr [sta::capacitance_ui_sta $v_cap] / [sta::distance_ui_sta 1.0]]
    }
  }

  sta::check_argc_eq0 "set_wire_rc" $args

  set signal [info exists flags(-signal)]
  set clk [info exists flags(-clock)]
  if { !$signal && !$clk } {
    set signal 1
    set clk 1
  }

  if { $signal && $clk } {
    set signal_clk "Signal/clock"
  } elseif { $signal } {
    set signal_clk "Signal"
  } elseif { $clk } {
    set signal_clk "Clock"
  }

  if { $h_wire_res == 0.0 } {
    utl::warn EST 10 "$signal_clk horizontal wire resistance is 0."
  }
  if { $v_wire_res == 0.0 } {
    utl::warn EST 11 "$signal_clk vertical wire resistance is 0."
  }
  if { $h_wire_cap == 0.0 } {
    utl::warn EST 12 "$signal_clk horizontal wire capacitance is 0."
  }
  if { $v_wire_cap == 0.0 } {
    utl::warn EST 13 "$signal_clk vertical wire capacitance is 0."
  }

  set corners $corner
  if { $corner == "NULL" } {
    set corners [sta::scenes]
  }
  foreach corner $corners {
    if { $signal } {
      est::set_h_wire_signal_rc_cmd $corner $h_wire_res $h_wire_cap
      est::set_v_wire_signal_rc_cmd $corner $v_wire_res $v_wire_cap
    }
    if { $clk } {
      est::set_h_wire_clk_rc_cmd $corner $h_wire_res $h_wire_cap
      est::set_v_wire_clk_rc_cmd $corner $v_wire_res $v_wire_cap
    }
  }
}

namespace eval est {
proc check_corner_wire_caps { } {
  set have_rc 1
  foreach corner [sta::scenes] {
    if { [est::wire_signal_capacitance $corner] == 0.0 } {
      utl::warn EST 18 "wire capacitance for corner $corner is zero.\
        Use the set_wire_rc command to set wire resistance and capacitance."
      set have_rc 0
    }
  }
  return $have_rc
}

proc check_parasitics { } {
  if { ![est::have_estimated_parasitics] } {
    utl::warn EST 27 "no estimated parasitics. Using wire load models."
  }
}

proc dblayer_wire_rc { layer } {
  set layer_width_dbu [$layer getWidth]
  set layer_width_micron [ord::dbu_to_microns $layer_width_dbu]
  set res_ohm_per_sq [$layer getResistance]
  set res_ohm_per_micron [expr $res_ohm_per_sq / $layer_width_micron]
  set cap_area_pf_per_sq_micron [$layer getCapacitance]
  set cap_edge_pf_per_micron [$layer getEdgeCapacitance]
  set cap_pf_per_micron [expr 1 * $layer_width_micron * $cap_area_pf_per_sq_micron \
    + $cap_edge_pf_per_micron * 2]
  # ohms/meter
  set wire_res [expr $res_ohm_per_micron * 1e+6]
  # farads/meter
  set wire_cap [expr $cap_pf_per_micron * 1e-12 * 1e+6]
  return [list $wire_res $wire_cap]
}

# Set DB layer RC
proc set_dblayer_wire_rc { layer res cap } {
  # Zero the edge cap and just use the user given value
  $layer setEdgeCapacitance 0
  set wire_width [ord::dbu_to_microns [$layer getWidth]]
  # Convert wire capacitance/wire_length to capacitance/area (pF/um)
  set cap_per_square [expr $cap * 1e+6 / $wire_width]
  $layer setCapacitance $cap_per_square

  # Convert resistance/wire_length (ohms/micron) to ohms/square
  set res_per_square [expr $wire_width * 1e-6 * $res]
  $layer setResistance $res_per_square
}

proc set_dbvia_wire_r { layer res } {
  $layer setResistance $res
}

# namespace
}

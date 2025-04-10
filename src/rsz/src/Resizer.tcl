# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

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
    utl::error "ORD" 201 "Use -layer or -via but not both."
  }

  set corners [sta::parse_corner_or_all keys]
  set tech [ord::get_db_tech]
  if { [info exists keys(-layer)] } {
    set layer_name $keys(-layer)
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "ORD" 202 "layer $layer_name not found."
    }

    if { [$layer getRoutingLevel] == 0 } {
      utl::error "ORD" 203 "$layer_name is not a routing layer."
    }

    if { ![info exists keys(-capacitance)] && ![info exists keys(-resistance)] } {
      utl::error "ORD" 204 "missing -capacitance or -resistance argument."
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
      set corners [sta::corners]
      # Only update the db layers if -corner not specified.
      rsz::set_dblayer_wire_rc $layer $res $cap
    }

    foreach corner $corners {
      rsz::set_layer_rc_cmd $layer $corner $res $cap
    }
  } elseif { [info exists keys(-via)] } {
    set layer_name $keys(-via)
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "ORD" 205 "via $layer_name not found."
    }

    if { [info exists keys(-capacitance)] } {
      utl::warn "ORD" 206 "-capacitance not supported for vias."
    }

    if { [info exists keys(-resistance)] } {
      set res $keys(-resistance)
      sta::check_positive_float "-resistance" $res
      set res [sta::resistance_ui_sta $res]

      if { $corners == "NULL" } {
        set corners [sta::corners]
        # Only update the db layers if -corner not specified.
        rsz::set_dbvia_wire_r $layer $res
      }

      foreach corner $corners {
        rsz::set_layer_rc_cmd $layer $corner $res 0.0
      }
    } else {
      utl::error "ORD" 208 "no -resistance specified for via."
    }
  } else {
    utl::error "ORD" 209 "missing -layer or -via argument."
  }
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

  set corner [sta::parse_corner_or_all keys]

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
      utl::error RSZ 1 "Use -layers or -resistance/-capacitance but not both."
    }
    if { [info exists keys(-layer)] } {
      utl::error RSZ 6 "Use -layers or -layer but not both."
    }
    set total_h_wire_res 0.0
    set total_h_wire_cap 0.0
    set total_v_wire_res 0.0
    set total_v_wire_cap 0.0

    set h_layers 0
    set v_layers 0

    set layers $keys(-layers)

    foreach layer_name $layers {
      set tec_layer [[ord::get_db_tech] findLayer $layer_name]
      if { $tec_layer == "NULL" } {
        utl::error RSZ 2 "layer $layer_name not found."
      }
      if { $corner == "NULL" } {
        lassign [rsz::dblayer_wire_rc $tec_layer] layer_wire_res layer_wire_cap
      } else {
        set layer_wire_res [rsz::layer_resistance $tec_layer $corner]
        set layer_wire_cap [rsz::layer_capacitance $tec_layer $corner]
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
    }
    if { $h_layers == 0 } {
      utl::error RSZ 16 "No horizontal layer specified."
    }
    if { $v_layers == 0 } {
      utl::error RSZ 17 "No vertical layer specified."
    }

    set h_wire_res [expr $total_h_wire_res / $h_layers]
    set h_wire_cap [expr $total_h_wire_cap / $h_layers]
    set v_wire_res [expr $total_v_wire_res / $v_layers]
    set v_wire_cap [expr $total_v_wire_cap / $v_layers]
  } elseif { [info exists keys(-layer)] } {
    set layer_name $keys(-layer)
    set tec_layer [[ord::get_db_tech] findLayer $layer_name]
    if { $tec_layer == "NULL" } {
      utl::error RSZ 15 "layer $tec_layer not found."
    }

    if { $corner == "NULL" } {
      lassign [rsz::dblayer_wire_rc $tec_layer] h_wire_res h_wire_cap
      lassign [rsz::dblayer_wire_rc $tec_layer] v_wire_res v_wire_cap
    } else {
      set h_wire_res [rsz::layer_resistance $tec_layer $corner]
      set v_wire_res [rsz::layer_resistance $tec_layer $corner]
      set h_wire_cap [rsz::layer_capacitance $tec_layer $corner]
      set v_wire_cap [rsz::layer_capacitance $tec_layer $corner]
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
    utl::warn RSZ 10 "$signal_clk horizontal wire resistance is 0."
  }
  if { $v_wire_res == 0.0 } {
    utl::warn RSZ 11 "$signal_clk vertical wire resistance is 0."
  }
  if { $h_wire_cap == 0.0 } {
    utl::warn RSZ 12 "$signal_clk horizontal wire capacitance is 0."
  }
  if { $v_wire_cap == 0.0 } {
    utl::warn RSZ 13 "$signal_clk vertical wire capacitance is 0."
  }

  set corners $corner
  if { $corner == "NULL" } {
    set corners [sta::corners]
  }
  foreach corner $corners {
    if { $signal } {
      rsz::set_h_wire_signal_rc_cmd $corner $h_wire_res $h_wire_cap
      rsz::set_v_wire_signal_rc_cmd $corner $v_wire_res $v_wire_cap
    }
    if { $clk } {
      rsz::set_h_wire_clk_rc_cmd $corner $h_wire_res $h_wire_cap
      rsz::set_v_wire_clk_rc_cmd $corner $v_wire_res $v_wire_cap
    }
  }
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
    if { [rsz::check_corner_wire_cap] } {
      rsz::estimate_parasitics_cmd "placement" $filename
    }
  } elseif { [info exists flags(-global_routing)] } {
    if { [grt::have_routes] } {
      # should check for layer rc
      rsz::estimate_parasitics_cmd "global_routing" $filename
    } else {
      utl::error RSZ 5 "Run global_route before estimating parasitics for global routing."
    }
  } else {
    utl::error RSZ 3 "missing -placement or -global_routing flag."
  }
}

sta::define_cmd_args "set_dont_use" {lib_cells}

proc set_dont_use { args } {
  sta::parse_key_args "set_dont_use" args keys {} flags {}
  set_dont_use_cmd "set_dont_use" $args 1
}

sta::define_cmd_args "unset_dont_use" {lib_cells}

proc unset_dont_use { args } {
  sta::parse_key_args "unset_dont_use" args keys {} flags {}
  set_dont_use_cmd "unset_dont_use" $args 0
}

sta::define_cmd_args "reset_dont_use" {}

proc reset_dont_use { args } {
  sta::parse_key_args "reset_dont_use" args keys {} flags {}
  rsz::reset_dont_use
}

proc set_dont_use_cmd { cmd cmd_args dont_use } {
  sta::check_argc_eq1 $cmd $cmd_args
  foreach lib_cell [sta::get_lib_cells_arg $cmd [lindex $cmd_args 0] sta::sta_warn] {
    rsz::set_dont_use $lib_cell $dont_use
  }
}

sta::define_cmd_args "set_dont_touch" {nets_instances}

proc set_dont_touch { args } {
  sta::parse_key_args "set_dont_touch" args keys {} flags {}
  set_dont_touch_cmd "set_dont_touch" $args 1
}

sta::define_cmd_args "unset_dont_touch" {nets_instances}

proc unset_dont_touch { args } {
  sta::parse_key_args "unset_dont_touch" args keys {} flags {}
  set_dont_touch_cmd "unset_dont_touch" $args 0
}

proc set_dont_touch_cmd { cmd cmd_args dont_touch } {
  sta::check_argc_eq1 $cmd $cmd_args
  sta::parse_inst_net_arg [lindex $cmd_args 0] insts nets
  foreach inst $insts {
    rsz::set_dont_touch_instance $inst $dont_touch
  }
  foreach net $nets {
    rsz::set_dont_touch_net $net $dont_touch
  }
}

sta::define_cmd_args "report_dont_use" {}

proc report_dont_use { args } {
  sta::parse_key_args "report_dont_use" args keys {} flags {}
  sta::check_argc_eq0 "report_dont_use" $args

  rsz::report_dont_use
}

sta::define_cmd_args "report_dont_touch" {}

proc report_dont_touch { args } {
  sta::parse_key_args "report_dont_touch" args keys {} flags {}
  sta::check_argc_eq0 "report_dont_touch" $args

  rsz::report_dont_touch
}

sta::define_cmd_args "buffer_ports" {[-inputs] [-outputs]\
                                       [-max_utilization util]\
                                       [-buffer_cell buf_cell]}

proc buffer_ports { args } {
  sta::parse_key_args "buffer_ports" args \
    keys {-buffer_cell -max_utilization} \
    flags {-inputs -outputs}

  set buffer_inputs [info exists flags(-inputs)]
  set buffer_outputs [info exists flags(-outputs)]
  if { !$buffer_inputs && !$buffer_outputs } {
    set buffer_inputs 1
    set buffer_outputs 1
  }
  sta::check_argc_eq0 "buffer_ports" $args

  rsz::set_max_utilization [rsz::parse_max_util keys]
  if { $buffer_inputs } {
    rsz::buffer_inputs
  }
  if { $buffer_outputs } {
    rsz::buffer_outputs
  }
}

sta::define_cmd_args "remove_buffers" { instances }

proc remove_buffers { args } {
  sta::parse_key_args "remove_buffers" args keys {} flags {}
  rsz::remove_buffers_cmd [get_cells $args]
}

sta::define_cmd_args "balance_row_usage" {}

proc balance_row_usage { args } {
  sta::parse_key_args "balance_row_usage" args keys {} flags {}
  sta::check_argc_eq0 "balance_row_usage" $args
  rsz::balance_row_usage_cmd
}

sta::define_cmd_args "repair_design" {[-max_wire_length max_wire_length] \
                                      [-max_utilization util] \
                                      [-slew_margin slack_margin] \
                                      [-cap_margin cap_margin] \
                                      [-buffer_gain gain] \
                                      [-pre_placement] \
                                      [-match_cell_footprint] \
                                      [-verbose]}

proc repair_design { args } {
  sta::parse_key_args "repair_design" args \
    keys {-max_wire_length -max_utilization -slew_margin -cap_margin -buffer_gain} \
    flags {-match_cell_footprint -verbose -pre_placement}

  set max_wire_length [rsz::parse_max_wire_length keys]
  set slew_margin [rsz::parse_percent_margin_arg "-slew_margin" keys]
  set cap_margin [rsz::parse_percent_margin_arg "-cap_margin" keys]

  set pre_placement [info exists flags(-pre_placement)]
  if { [info exists keys(-buffer_gain)] } {
    set pre_placement true
    utl::warn "RSZ" 149 "-buffer_gain is deprecated"
  }
  rsz::set_max_utilization [rsz::parse_max_util keys]

  sta::check_argc_eq0 "repair_design" $args
  rsz::check_parasitics
  set max_wire_length [rsz::check_max_wire_length $max_wire_length]
  set match_cell_footprint [info exists flags(-match_cell_footprint)]
  set verbose [info exists flags(-verbose)]
  rsz::repair_design_cmd $max_wire_length $slew_margin $cap_margin \
    $pre_placement $match_cell_footprint $verbose
}

sta::define_cmd_args "repair_clock_nets" {[-max_wire_length max_wire_length]}

proc repair_clock_nets { args } {
  sta::parse_key_args "repair_clock_nets" args \
    keys {-max_wire_length} \
    flags {}

  set max_wire_length [rsz::parse_max_wire_length keys]


  sta::check_argc_eq0 "repair_clock_nets" $args
  rsz::check_parasitics
  set max_wire_length [rsz::check_max_wire_length $max_wire_length]
  rsz::repair_clk_nets_cmd $max_wire_length
}

sta::define_cmd_args "repair_clock_inverters" {}

proc repair_clock_inverters { args } {
  sta::parse_key_args "repair_clock_inverters" args keys {} flags {}
  sta::check_argc_eq0 "repair_clock_inverters" $args
  rsz::repair_clk_inverters_cmd
}

sta::define_cmd_args "repair_tie_fanout" {lib_port\
                                         [-separation dist]\
                                         [-max_fanout fanout]\
                                         [-verbose]}

proc repair_tie_fanout { args } {
  sta::parse_key_args "repair_tie_fanout" args keys {-separation -max_fanout} \
    flags {-verbose}

  set separation 0
  if { [info exists keys(-separation)] } {
    set separation $keys(-separation)
    sta::check_positive_float "-separation" $separation
    set separation [sta::distance_ui_sta $separation]
  }
  set verbose [info exists flags(-verbose)]

  sta::check_argc_eq1 "repair_tie_fanout" $args
  set lib_port [lindex $args 0]
  if { ![sta::is_object $lib_port] } {
    set lib_port [get_lib_pins [lindex $args 0]]
    if { [llength $lib_port] > 1 } {
      # multiple libraries match the lib port arg; use any
      set lib_port [lindex $lib_port 0]
    }
  }
  if { $lib_port != "" } {
    rsz::repair_tie_fanout_cmd $lib_port $separation $verbose
  }
}


# -max_passes is for developer debugging so intentionally not documented
# in define_cmd_args
sta::define_cmd_args "repair_timing" {[-setup] [-hold]\
                                        [-recover_power percent_of_paths_with_slack]\
                                        [-setup_margin setup_margin]\
                                        [-hold_margin hold_margin]\
                                        [-slack_margin slack_margin]\
                                        [-libraries libs]\
                                        [-allow_setup_violations]\
                                        [-skip_pin_swap]\
                                        [-skip_gate_cloning]\
                                        [-skip_buffering]\
                                        [-skip_buffer_removal]\
                                        [-skip_last_gasp]\
                                        [-repair_tns tns_end_percent]\
                                        [-max_passes passes]\
                                        [-max_buffer_percent buffer_percent]\
                                        [-max_utilization util] \
                                        [-match_cell_footprint] \
                                        [-max_repairs_per_pass max_repairs_per_pass]\
                                        [-verbose]}

proc repair_timing { args } {
  sta::parse_key_args "repair_timing" args \
    keys {-setup_margin -hold_margin -slack_margin \
            -libraries -max_utilization -max_buffer_percent \
            -recover_power -repair_tns -max_passes -max_repairs_per_pass} \
    flags {-setup -hold -allow_setup_violations -skip_pin_swap -skip_gate_cloning \
           -skip_buffering -skip_buffer_removal -skip_last_gasp -match_cell_footprint \
           -verbose}

  set setup [info exists flags(-setup)]
  set hold [info exists flags(-hold)]

  if { !$setup && !$hold } {
    set setup 1
    set hold 1
  }

  if { [info exists keys(-slack_margin)] } {
    utl::warn RSZ 76 "-slack_margin is deprecated. Use -setup_margin/-hold_margin"
    if { !$setup && $hold } {
      set setup_margin 0.0
      set hold_margin [rsz::parse_time_margin_arg "-slack_margin" keys]
    } else {
      set setup_margin [rsz::parse_time_margin_arg "-slack_margin" keys]
      set hold_margin 0.0
    }
  } else {
    set setup_margin [rsz::parse_time_margin_arg "-setup_margin" keys]
    set hold_margin [rsz::parse_time_margin_arg "-hold_margin" keys]
  }

  set allow_setup_violations [info exists flags(-allow_setup_violations)]
  set skip_pin_swap [info exists flags(-skip_pin_swap)]
  set skip_gate_cloning [info exists flags(-skip_gate_cloning)]
  set skip_buffering [info exists flags(-skip_buffering)]
  set skip_buffer_removal [info exists flags(-skip_buffer_removal)]
  set skip_last_gasp [info exists flags(-skip_last_gasp)]
  rsz::set_max_utilization [rsz::parse_max_util keys]

  set max_buffer_percent 20
  if { [info exists keys(-max_buffer_percent)] } {
    set max_buffer_percent $keys(-max_buffer_percent)
    sta::check_percent "-max_buffer_percent" $max_buffer_percent
  }
  set max_buffer_percent [expr $max_buffer_percent / 100.0]

  set repair_tns_end_percent 1.0
  if { [info exists keys(-repair_tns)] } {
    set repair_tns_end_percent $keys(-repair_tns)
    sta::check_percent "-repair_tns" $repair_tns_end_percent
    set repair_tns_end_percent [expr $repair_tns_end_percent / 100.0]
  }

  set recover_power_percent -1
  if { [info exists keys(-recover_power)] } {
    set recover_power_percent $keys(-recover_power)
    sta::check_percent "-recover_power" $recover_power_percent
    set recover_power_percent [expr $recover_power_percent / 100.0]
  }

  set verbose 0
  if { [info exists flags(-verbose)] } {
    set verbose 1
  }

  set max_passes 10000
  if { [info exists keys(-max_passes)] } {
    set max_passes $keys(-max_passes)
  }

  set match_cell_footprint [info exists flags(-match_cell_footprint)]
  if { [design_is_routed] } {
    rsz::set_parasitics_src "detailed_routing"
  }

  set max_repairs_per_pass 1
  if { [info exists keys(-max_repairs_per_pass)] } {
    set max_repairs_per_pass $keys(-max_repairs_per_pass)
  }

  sta::check_argc_eq0 "repair_timing" $args
  rsz::check_parasitics

  set recovered_power 0
  set repaired_setup 0
  set repaired_hold 0
  if { $recover_power_percent >= 0 } {
    set recovered_power [rsz::recover_power $recover_power_percent $match_cell_footprint $verbose]
  } else {
    if { $setup } {
      set repaired_setup [rsz::repair_setup $setup_margin $repair_tns_end_percent $max_passes \
        $max_repairs_per_pass $match_cell_footprint $verbose \
        $skip_pin_swap $skip_gate_cloning $skip_buffering \
        $skip_buffer_removal $skip_last_gasp]
    }
    if { $hold } {
      set repaired_hold [rsz::repair_hold $setup_margin $hold_margin \
        $allow_setup_violations $max_buffer_percent $max_passes \
        $match_cell_footprint $verbose]
    }
  }

  return [expr $recovered_power || $repaired_setup || $repaired_hold]
}

################################################################

sta::define_cmd_args "report_design_area" {}

proc report_design_area { args } {
  sta::parse_key_args "report_design_area" args keys {} flags {}
  set util [format %.0f [expr [rsz::utilization] * 100]]
  set area [sta::format_area [rsz::design_area] 0]
  utl::report "Design area ${area} u^2 ${util}% utilization."
}

sta::define_cmd_args "report_floating_nets" {[-verbose] [> filename] [>> filename]} ;# checker off

sta::proc_redirect report_floating_nets {
  sta::parse_key_args "report_floating_nets" args keys {} flags {-verbose};# checker off

  set verbose [info exists flags(-verbose)]
  set floating_nets [rsz::find_floating_nets]
  set floating_pins [rsz::find_floating_pins]
  set floating_net_count [llength $floating_nets]
  set floating_pin_count [llength $floating_pins]
  if { $floating_net_count > 0 } {
    utl::warn RSZ 20 "found $floating_net_count floating nets."
    if { $verbose } {
      foreach net $floating_nets {
        utl::report " [get_full_name $net]"
      }
    }
  }
  if { $floating_pin_count > 0 } {
    utl::warn RSZ 95 "found $floating_pin_count floating pins."
    if { $verbose } {
      foreach pin $floating_pins {
        utl::report " [get_full_name $pin]"
      }
    }
  }

  utl::metric_int "timing__drv__floating__nets" $floating_net_count
  utl::metric_int "timing__drv__floating__pins" $floating_pin_count
}

sta::define_cmd_args "report_overdriven_nets" {[-include_parallel_driven] \
                                               [-verbose] \
                                               [> filename] \
                                               [>> filename]} ;# checker off

sta::proc_redirect report_overdriven_nets {
  sta::parse_key_args "report_overdriven_nets" args \
    keys {} \
    flags {-verbose -include_parallel_driven};# checker off

  set verbose [info exists flags(-verbose)]
  set overdriven_nets [rsz::find_overdriven_nets [info exists flags(-include_parallel_driven)]]
  set overdriven_net_count [llength $overdriven_nets]
  if { $overdriven_net_count > 0 } {
    utl::warn RSZ 24 "found $overdriven_net_count overdriven nets."
    if { $verbose } {
      foreach net $overdriven_nets {
        utl::report " [get_full_name $net]"
      }
    }
  }

  utl::metric_int "timing__drv__overdriven__nets" $overdriven_net_count
}

sta::define_cmd_args "report_long_wires" {count [> filename] [>> filename]} ;# checker off

sta::proc_redirect report_long_wires {
  global sta_report_default_digits

  sta::parse_key_args "report_long_wires" args keys {-digits} flags {};# checker off

  set digits $sta_report_default_digits
  if { [info exists keys(-digits)] } {
    set digits $keys(-digits)
  }

  sta::check_argc_eq1 "report_long_wires" $args
  set count [lindex $args 0]
  rsz::report_long_wires_cmd $count $digits
}

sta::define_cmd_args "eliminate_dead_logic" {}
proc eliminate_dead_logic { } {
  rsz::eliminate_dead_logic_cmd 1
}

namespace eval rsz {
proc get_block { } {
  set db [ord::get_db]
  if { $db eq "NULL" } {
    utl::error "RSZ" 200 "db needs to be defined for set_opt_config and related commands."
  }
  set chip [$db getChip]
  if { $chip eq "NULL" } {
    utl::error "RSZ" 201 "chip needs to be defined for set_opt_config and related commands."
  }
  set block [$chip getBlock]
  if { $block eq "NULL" } {
    utl::error "RSZ" 202 "block needs to be defined for set_opt_config and related commands."
  }
  return $block
}

proc set_positive_double_prop { value opt_name prop_name } {
  sta::check_positive_float $opt_name $value
  set block [get_block]
  set prop [odb::dbDoubleProperty_find $block $prop_name]
  if { $prop eq "NULL" } {
    odb::dbDoubleProperty_create $block $prop_name $value
  } else {
    $prop setValue $value
  }
}

proc set_boolean_prop { value opt_name prop_name } {
  if { ![string is boolean $value] } {
    utl::error "RSZ" 209 \
      "$opt_name argument should be Boolean"
  }
  set block [get_block]
  set prop [odb::dbBoolProperty_find $block $prop_name]
  if { $prop eq "NULL" } {
    odb::dbBoolProperty_create $block $prop_name $value
  } else {
    $prop setValue $value
  }
}

proc clear_double_prop { name } {
  set block [get_block]
  set prop [odb::dbDoubleProperty_find $block $name]
  if { $prop ne "NULL" && $prop ne "" } {
    odb::dbProperty_destroy $prop
  }
}

proc clear_bool_prop { name } {
  set block [get_block]
  set prop [odb::dbBoolProperty_find $block $name]
  if { $prop ne "NULL" && $prop ne "" } {
    odb::dbProperty_destroy $prop
  }
}
}

sta::define_cmd_args "set_opt_config" { [-limit_sizing_area] \
                                          [-limit_sizing_leakage] \
                                          [-keep_sizing_site] \
                                          [-keep_sizing_vt] \
                                          [-sizing_area_limit] \
                                          [-sizing_leakage_limit] \
                                          [-set_early_sizing_cap_ratio] \
                                          [-set_early_buffer_sizing_cap_ratio]}

proc set_opt_config { args } {
  sta::parse_key_args "set_opt_config" args \
    keys {-limit_sizing_area -limit_sizing_leakage -sizing_area_limit \
      -sizing_leakage_limit -keep_sizing_site -keep_sizing_vt \
      -set_early_sizing_cap_ratio -set_early_buffer_sizing_cap_ratio} flags {}

  set area_limit "NULL"
  if { [info exists keys(-limit_sizing_area)] } {
    set area_limit $keys(-limit_sizing_area)
  } elseif { [info exists keys(-sizing_area_limit)] } {
    set area_limit $keys(-sizing_area_limit)
  }
  if { $area_limit ne "NULL" } {
    rsz::set_positive_double_prop \
      $area_limit "-sizing_area_limit" "limit_sizing_area"
    utl::info RSZ 100 \
      "Cells with area > ${area_limit}X current cell will not be considered for sizing"
  }

  set leakage_limit "NULL"
  if { [info exists keys(-limit_sizing_leakage)] } {
    set leakage_limit $keys(-limit_sizing_leakage)
  } elseif { [info exists keys(-sizing_leakage_limit)] } {
    set leakage_limit $keys(-sizing_leakage_limit)
  }
  if { $leakage_limit ne "NULL" } {
    rsz::set_positive_double_prop \
      $leakage_limit "-sizing_leakage_limit" "limit_sizing_leakage"
    utl::info RSZ 101 \
      "Cells with leakage > ${leakage_limit}X current cell will not be considered for sizing"
  }

  if { [info exists keys(-keep_sizing_site)] } {
    rsz::set_boolean_prop $keys(-keep_sizing_site) "-keep_sizing_site" "keep_sizing_site"
    utl::info RSZ 104 \
      "Cell's site will be preserved for sizing"
  }

  if { [info exists keys(-keep_sizing_vt)] } {
    rsz::set_boolean_prop $keys(-keep_sizing_vt) "-keep_sizing_vt" "keep_sizing_vt"
    utl::info RSZ 106 \
      "Cell's VT type will be preserved for sizing"
  }

  if { [info exists keys(-set_early_sizing_cap_ratio)] } {
    set value $keys(-set_early_sizing_cap_ratio)
    rsz::set_positive_double_prop $value "-set_early_sizing_cap_ratio" "early_sizing_cap_ratio"
    utl::info RSZ 145 \
      "Early cell sizing will use capacitance ratio of value $value"
  }

  if { [info exists keys(-set_early_buffer_sizing_cap_ratio)] } {
    set value $keys(-set_early_buffer_sizing_cap_ratio)
    rsz::set_positive_double_prop $value "-set_early_buffer_sizing_cap_ratio" \
      "early_buffer_sizing_cap_ratio"
    utl::info RSZ 147 \
      "Early buffer sizing will use capacitance ratio of value $value"
  }
}

sta::define_cmd_args "reset_opt_config" { [-limit_sizing_area] \
                                            [-limit_sizing_leakage] \
                                            [-keep_sizing_site] \
                                            [-keep_sizing_vt] \
                                            [-sizing_area_limit] \
                                            [-sizing_leakage_limit] \
                                            [-set_early_sizing_cap_ratio] \
                                            [-set_early_buffer_sizing_cap_ratio]}

proc reset_opt_config { args } {
  sta::parse_key_args "reset_opt_config" args \
    keys {} flags {-limit_sizing_area -limit_sizing_leakage -keep_sizing_site \
                     -sizing_area_limit -sizing_leakage_limit -keep_sizing_vt \
                     -set_early_sizing_cap_ratio -set_early_buffer_sizing_cap_ratio}
  set reset_all [expr { [array size flags] == 0 }]

  if {
    $reset_all || [info exists flags(-limit_sizing_area)]
    || [info exists flags(-sizing_area_limit)]
  } {
    rsz::clear_double_prop "limit_sizing_area"
    utl::info RSZ 102 "Cell sizing restriction based on area has been removed."
  }
  if {
    $reset_all || [info exists flags(-limit_sizing_leakage)]
    || [info exists flags(-sizing_leakage_limit)]
  } {
    rsz::clear_double_prop "limit_sizing_leakage"
    utl::info RSZ 103 "Cell sizing restriction based on leakage has been removed."
  }
  if { $reset_all || [info exists flags(-keep_sizing_site)] } {
    rsz::clear_bool_prop "keep_sizing_site"
    utl::info RSZ 105 "Cell sizing restriction based on site has been removed."
  }
  if { $reset_all || [info exists flags(-keep_sizing_vt)] } {
    rsz::clear_bool_prop "keep_sizing_vt"
    utl::info RSZ 107 "Cell sizing restriction based on VT type has been removed."
  }
  if { $reset_all || [info exists flags(-set_early_sizing_cap_ratio)] } {
    rsz::clear_double_prop "early_sizing_cap_ratio"
    utl::info RSZ 146 "Capacitance ratio for early cell sizing has been unset."
  }
  if { $reset_all || [info exists flags(-set_early_buffer_sizing_cap_ratio)] } {
    rsz::clear_double_prop "early_buffer_sizing_cap_ratio"
    utl::info RSZ 148 "Capacitance ratio for early buffer sizing has been unset."
  }
}

sta::define_cmd_args "report_opt_config" {}

proc report_opt_config { args } {
  sta::parse_key_args "report_opt_config" args keys {} flags {}
  set block [rsz::get_block]

  set area_limit_value "undefined"
  set area_limit [odb::dbDoubleProperty_find $block "limit_sizing_area"]
  if { $area_limit ne "NULL" && $area_limit ne "" } {
    set area_limit_value [$area_limit getValue]
  }

  set leakage_limit_value "undefined"
  set leakage_limit [odb::dbDoubleProperty_find $block "limit_sizing_leakage"]
  if { $leakage_limit ne "NULL" && $leakage_limit ne "" } {
    set leakage_limit_value [$leakage_limit getValue]
  }

  set keep_site_value "false"
  set keep_site [odb::dbBoolProperty_find $block "keep_sizing_site"]
  if { $keep_site ne "NULL" && $keep_site ne "" } {
    set keep_site_result [$keep_site getValue]
    set keep_site_value [expr { $keep_site_result ? "true" : "false" }]
  }

  set keep_sizing_vt "false"
  set keep_vt [odb::dbBoolProperty_find $block "keep_sizing_vt"]
  if { $keep_vt ne "NULL" && $keep_vt ne "" } {
    set keep_vt_value [$keep_vt getValue]
    set keep_sizing_vt [expr { $keep_vt_value ? "true" : "false" }]
  }

  set sizing_cap_ratio "unset"
  set cap_ratio_prop [odb::dbDoubleProperty_find $block "early_sizing_cap_ratio"]
  if { $cap_ratio_prop ne "NULL" && $cap_ratio_prop ne "" } {
    set sizing_cap_ratio [$cap_ratio_prop getValue]
  }

  set buffer_cap_ratio "unset"
  set buffer_cap_ratio_prop [odb::dbDoubleProperty_find $block "early_buffer_sizing_cap_ratio"]
  if { $buffer_cap_ratio_prop ne "NULL" && $buffer_cap_ratio_prop ne "" } {
    set buffer_cap_ratio [$buffer_cap_ratio_prop getValue]
  }

  puts "*******************************************"
  puts "Optimization config:"
  puts "-limit_sizing_area:                 $area_limit_value"
  puts "-limit_sizing_leakage:              $leakage_limit_value"
  puts "-keep_sizing_site:                  $keep_site_value"
  puts "-keep_sizing_vt:                    $keep_sizing_vt"
  puts "-set_early_sizing_cap_ratio:        $sizing_cap_ratio"
  puts "-set_early_buffer_sizing_cap_ratio: $buffer_cap_ratio"
  puts "*******************************************"
}

sta::define_cmd_args "report_equiv_cells" { -match_cell_footprint -all }

proc report_equiv_cells { args } {
  sta::parse_key_args "report_equiv_cells" args keys {} flags {-match_cell_footprint -all}
  set match_cell_footprint [info exists flags(-match_cell_footprint)]
  set report_all_cells [info exists flags(-all)]
  sta::check_argc_eq1 "report_equiv_cells" $args
  set lib_cell [sta::get_lib_cells_arg "report_equiv_cells" [lindex $args 0] sta::sta_warn]
  rsz::report_equiv_cells_cmd $lib_cell $match_cell_footprint $report_all_cells
}

namespace eval rsz {
# for testing
proc repair_setup_pin { end_pin } {
  check_parasitics
  repair_setup_pin_cmd $end_pin
}

proc set_debug { args } {
  sta::parse_key_args "set_debug" args \
    keys { -net } \
    flags { -subdivide_step } ;# checker off

  set net ""
  if { [info exists keys(-net)] } {
    set net $keys(-net)
  }

  set subdivide_step [info exists flags(-subdivide_step)]

  rsz::set_debug_cmd $net $subdivide_step
}

proc report_swappable_pins { } {
  report_swappable_pins_cmd
}

proc check_parasitics { } {
  if { ![have_estimated_parasitics] } {
    utl::warn RSZ 21 "no estimated parasitics. Using wire load models."
  }
}

proc parse_time_margin_arg { key keys_var } {
  return [sta::time_ui_sta [parse_margin_arg $key $keys_var]]
}

proc parse_percent_margin_arg { key keys_var } {
  set margin [parse_margin_arg $key $keys_var]
  if { !($margin >= 0 && $margin < 100) } {
    utl::warn RSZ 67 "$key must be  between 0 and 100 percent."
  }
  return $margin
}

proc parse_margin_arg { key keys_var } {
  upvar 2 $keys_var keys

  set margin 0.0
  if { [info exists keys($key)] } {
    set margin $keys($key)
    sta::check_float $key $margin
  }
  return $margin
}

proc parse_max_util { keys_var } {
  upvar 1 $keys_var keys
  set max_util 0.0
  if { [info exists keys(-max_utilization)] } {
    set max_util $keys(-max_utilization)
    if { !([string is double $max_util] && $max_util >= 0.0 && $max_util <= 100) } {
      utl::error RSZ 4 "-max_utilization must be between 0 and 100%."
    }
    set max_util [expr $max_util / 100.0]
  }
  return $max_util
}

proc parse_max_wire_length { keys_var } {
  upvar 1 $keys_var keys
  set max_wire_length 0
  if { [info exists keys(-max_wire_length)] } {
    set max_wire_length $keys(-max_wire_length)
    sta::check_positive_float "-max_wire_length" $max_wire_length
    set max_wire_length [sta::distance_ui_sta $max_wire_length]
  }
  return $max_wire_length
}

proc check_corner_wire_caps { } {
  set have_rc 1
  foreach corner [sta::corners] {
    if { [rsz::wire_signal_capacitance $corner] == 0.0 } {
      utl::warn RSZ 18 "wire capacitance for corner [$corner name] is zero.\
        Use the set_wire_rc command to set wire resistance and capacitance."
      set have_rc 0
    }
  }
  return $have_rc
}

proc check_max_wire_length { max_wire_length } {
  if { [rsz::wire_signal_resistance [sta::cmd_corner]] > 0 } {
    set min_delay_max_wire_length [rsz::find_max_wire_length]
    if { $max_wire_length > 0 } {
      if { $max_wire_length < $min_delay_max_wire_length } {
        set wire_length_fmt [format %.0fu [sta::distance_sta_ui $min_delay_max_wire_length]]
        utl::warn RSZ 65 "max wire length less than $wire_length_fmt increases wire delays."
      }
    } else {
      set max_wire_length $min_delay_max_wire_length
      set max_wire_length_fmt [format %.0f [sta::distance_sta_ui $max_wire_length]]
      utl::info RSZ 58 "Using max wire length ${max_wire_length_fmt}um."
    }
  }
  return $max_wire_length
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

############################################################################
##
## Copyright (c) 2019, OpenROAD
## All rights reserved.
##
## BSD 3-Clause License
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
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################

sta::define_cmd_args "remove_buffers" {}

proc remove_buffers { args } {
  sta::check_argc_eq0 "remove_buffers" $args
  rsz::remove_buffers_cmd
}

sta::define_cmd_args "set_wire_rc" {[-clock] [-signal]\
                                      [-layer layer_name]\
                                      [-layers {layer_name weight ...}]\
                                      [-resistance res ][-capacitance cap]}

proc set_wire_rc { args } {
   sta::parse_key_args "set_wire_rc" args \
     keys {-layer -layers -resistance -capacitance -corner} \
     flags {-clock -signal -data}

  set wire_res 0.0
  set wire_cap 0.0

  if { [info exists keys(-layer)] } {
    if { [info exists keys(-resistance)] \
           || [info exists keys(-capacitance)] } {
      utl::error RSZ 1 "Use -layer or -resistance/-capacitance but not both."
    }
    set layer_name $keys(-layer)
    lassign [rsz::layer_wire_rc $layer_name] wire_res wire_cap
    set report_rc 1
  } elseif { [info exists keys(-layers)] } {
    set layers $keys(-layers)
    set res_sum 0.0
    set cap_sum 0.0
    set weigth_sum 0.0
    foreach {layer_name weight} $layers {
      lassign [rsz::layer_wire_rc $layer_name] layer_res layer_cap
      sta::check_positive_float "layer weight" $weight
      set res_sum [expr $res_sum + $layer_res * $weight]
      set cap_sum [expr $cap_sum + $layer_cap * $weight]
      set weigth_sum [expr $weigth_sum + $weight]
    }
    set wire_res [expr $res_sum / $weigth_sum]
    set wire_cap [expr $cap_sum / $weigth_sum]
    set report_rc 1
  } else {
    ord::ensure_units_initialized
    if { [info exists keys(-resistance)] } {
      set res $keys(-resistance)
      sta::check_positive_float "-resistance" $res
      set wire_res [expr [sta::resistance_ui_sta $res] / [sta::distance_ui_sta 1.0]]
    }
    
    if { [info exists keys(-capacitance)] } {
      set cap $keys(-capacitance)
      sta::check_positive_float "-capacitance" $cap
      set wire_cap [expr [sta::capacitance_ui_sta $cap] / [sta::distance_ui_sta 1.0]]
    }
    set report_rc 0
  }
  
  if { [info exists keys(-corner)] } {
    utl::warn RSZ 12 "-corner argument ignored."
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

  # Unfortunately this does not work very well with technologies like sky130
  # that use inappropriate kohm/pf units.
  set report_rc 0
  if { $report_rc } {
    utl::info RSZ 61 "$signal_clk wire resistance [sta::format_resistance [expr $wire_res * 1e-6] 6] [sta::unit_scale_abreviation resistance][sta::unit_suffix resistance]/um capacitance [sta::format_capacitance [expr $wire_cap * 1e-6] 6] [sta::unit_scale_abreviation capacitance][sta::unit_suffix capacitance]/um."
  }

  if { $wire_res == 0.0 } {
    utl::warn RSZ 10 "$signal_clk wire resistance is 0."
  }
  if { $wire_cap == 0.0 } {
    utl::warn RSZ 11 "$signal_clk wire capacitance is 0."
  }

  if { $signal } {
    rsz::set_wire_rc_cmd $wire_res $wire_cap
  }
  if { $clk } {
    rsz::set_wire_clk_rc_cmd $wire_res $wire_cap
  }
}

sta::define_cmd_args "estimate_parasitics" { -placement|-global_routing }

proc estimate_parasitics { args } {
  sta::parse_key_args "estimate_parasitics" args \
    keys {} flags {-placement -global_routing}

  sta::check_argc_eq0 "estimate_parasitics" $args
  if { [info exists flags(-placement)] } {
    if { [rsz::wire_capacitance] == 0.0 } {
      utl::warn RSZ 14 "wire capacitance is zero. Use the set_wire_rc command to set wire resistance and capacitance."
    } else {
      rsz::estimate_parasitics_cmd
    }
  } elseif { [info exists flags(-global_routing)] } {
    grt::estimate_rc_cmd
  } else {
    utl::error RSZ 3 "missing -placement or -global_routing flag."
  }
}

sta::define_cmd_args "set_dont_use" {lib_cells}

proc set_dont_use { args } {
  sta::check_argc_eq1 "set_dont_use" $args
  rsz::set_dont_use_cmd [sta::get_lib_cells_arg "-dont_use" [lindex $args 0] utl::warn]
}

sta::define_cmd_args "buffer_ports" {[-inputs] [-outputs]\
                                       [-max_utilization util]}

proc buffer_ports { args } {
  sta::parse_key_args "buffer_ports" args \
    keys {-buffer_cell -max_utilization} \
    flags {-inputs -outputs}
  
  if { [info exists keys(-buffer_cell)] } {
    utl::warn RSZ 15 "-buffer_cell is deprecated."
  }

  set buffer_inputs [info exists flags(-inputs)]
  set buffer_outputs [info exists flags(-outputs)]
  if { !$buffer_inputs && !$buffer_outputs } {
    set buffer_inputs 1
    set buffer_outputs 1
  }
  sta::check_argc_eq0 "buffer_ports" $args
  
  rsz::set_max_utilization [rsz::parse_max_util keys]
  rsz::resizer_preamble [get_libs *]
  if { $buffer_inputs } {
    rsz::buffer_inputs
  }
  if { $buffer_outputs } {
    rsz::buffer_outputs
  }
}

sta::define_cmd_args "repair_design" {[-max_wire_length max_wire_length]\
                                        [-libraries resize_libs]\
                                        [-max_utilization util]}

proc repair_design { args } {
  sta::parse_key_args "repair_design" args \
    keys {-max_wire_length -buffer_cell -libraries -max_utilization} \
    flags {}
  
  if { [info exists keys(-buffer_cell)] } {
    utl::warn RSZ 16 "-buffer_cell is deprecated."
  }
  set max_wire_length [rsz::parse_max_wire_length keys]
  
  if { [info exists keys(-libraries)] } {
    set resize_libs [get_liberty_error "-libraries" $keys(-libraries)]
  } else {
    set resize_libs [get_libs *]
    if { $resize_libs == {} } {
      utl::error RSZ 8 "No liberty libraries found."
    }
  }
  rsz::set_max_utilization [rsz::parse_max_util keys]

  sta::check_argc_eq0 "repair_design" $args
  rsz::check_parasitics
  rsz::resizer_preamble $resize_libs
  set max_wire_length [rsz::check_max_wire_length $max_wire_length]
  rsz::repair_design_cmd $max_wire_length
}

sta::define_cmd_args "repair_clock_nets" {[-max_wire_length max_wire_length]}

proc repair_clock_nets { args } {
  sta::parse_key_args "repair_clock_nets" args \
    keys {-max_wire_length -buffer_cell} \
    flags {}
  
  if { [info exists keys(-buffer_cell)] } {
    utl::warn RSZ 18 "-buffer_cell is deprecated."
  }
  set max_wire_length [rsz::parse_max_wire_length keys]
  
  sta::check_argc_eq0 "repair_clock_nets" $args
  rsz::check_parasitics
  rsz::resizer_preamble [get_libs *]
  set max_wire_length [rsz::check_max_wire_length $max_wire_length]
  rsz::repair_clk_nets_cmd $max_wire_length
}

sta::define_cmd_args "repair_clock_inverters" {-buffer_cell buffer_cell}

proc repair_clock_inverters { args } {
  sta::check_argc_eq0 "repair_clock_inverters" $args
  rsz::repair_clk_inverters_cmd
}

sta::define_cmd_args "repair_tie_fanout" {lib_port [-separation dist] [-verbose]}

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
  }
  if { $lib_port != "" } {
    rsz::repair_tie_fanout_cmd $lib_port $separation $verbose
  }
}

sta::define_cmd_args "repair_hold_violations" {[-allow_setup_violations]}

proc repair_hold_violations { args } {
  sta::parse_key_args "repair_hold_violations" args \
    keys {-buffer_cell} \
    flags {-allow_setup_violations}
  
  set allow_setup_violations [info exists flags(-allow_setup_violations)]
  sta::check_argc_eq0 "repair_hold_violations" $args
  utl::warn RSZ 19 "repair_hold_violations is deprecated. Use repair_timing -hold"
  set resize_libs [get_libs *]
  if { $resize_libs == {} } {
    utl::error RSZ 9 "No liberty libraries found."
  }

  rsz::check_parasitics
  rsz::resizer_preamble $resize_libs
  rsz::repair_hold 0.0 $allow_setup_violations $max_buffer_percent
}

sta::define_cmd_args "repair_timing" {[-setup] [-hold]\
                                        [-slack_margin slack_margin]\
                                        [-max_buffer_percent buffer_percent]\
                                        [-allow_setup_violations]\
                                        [-libraries resize_libs]\
                                        [-max_utilization util]}

proc repair_timing { args } {
  sta::parse_key_args "repair_timing" args \
    keys {-slack_margin -libraries -max_utilization -max_buffer_percent} \
    flags {-setup -hold -allow_setup_violations}

  set setup [info exists flags(-setup)]
  set hold [info exists flags(-hold)]
  if { !$setup && !$hold } {
    set setup 1
    set hold 1
  }

  set slack_margin [rsz::parse_slack_margin_arg keys]
  if { [info exists keys(-libraries)] } {
    set resize_libs [get_liberty_error "-libraries" $keys(-libraries)]
  } else {
    set resize_libs [get_libs *]
    if { $resize_libs == {} } {
      utl::error RSZ 49 "No liberty libraries found."
    }
  }
  set allow_setup_violations [info exists flags(-allow_setup_violations)]
  rsz::set_max_utilization [rsz::parse_max_util keys]

  set max_buffer_percent 20
  if { [info exists keys(-max_buffer_percent)] } {
    set max_buffer_percent $keys(-max_buffer_percent)
    sta::check_positive_float "-max_buffer_percent" $max_buffer_percent
    set max_buffer_percent [expr $max_buffer_percent / 100.0]
  }

  sta::check_argc_eq0 "repair_timing" $args
  rsz::check_parasitics
  rsz::resizer_preamble $resize_libs
  if { $setup } {
    rsz::repair_setup $slack_margin
  }
  if { $hold } {
    rsz::repair_hold $slack_margin $allow_setup_violations $max_buffer_percent
  }
}

################################################################

sta::define_cmd_args "report_design_area" {}

proc report_design_area {} {
  set util [format %.0f [expr [rsz::utilization] * 100]]
  set area [sta::format_area [rsz::design_area] 0]
  utl::report "Design area ${area} u^2 ${util}% utilization."
}

sta::define_cmd_args "report_floating_nets" {[-verbose]}

proc report_floating_nets { args } {
  sta::parse_key_args "report_floating_nets" args keys {} flags {-verbose}
  
  set verbose [info exists flags(-verbose)]
  set floating_nets [rsz::find_floating_nets]
  set floating_net_count [llength $floating_nets]
  if { $floating_net_count > 0 } {
    utl::warn RSZ 20 "found $floating_net_count floatiing nets."
    if { $verbose } {
      foreach net $floating_nets {
        utl::report " [get_full_name $net]"
      }
    }
  }
}

sta::define_cmd_args "report_long_wires" {count}

sta::proc_redirect report_long_wires {
  global sta_report_default_digits

  sta::parse_key_args "report_long_wires" args keys {-digits} flags {}
  
  set digits $sta_report_default_digits
  if { [info exists keys(-digits)] } {
    set digits $keys(-digits)
  }

  sta::check_argc_eq1 "report_long_wires" $args
  set count [lindex $args 0]
  rsz::report_long_wires_cmd $count $digits
}

namespace eval rsz {

# for testing resizing separately
proc resize { args } {
  sta::check_argc_eq0 "resize" $args
  check_parasitics
  resizer_preamble [get_libs *]
  resize_to_target_slew
}

# for testing
proc repair_setup_pin { end_pin } {
  check_parasitics
  resizer_preamble [get_libs *]
  repair_setup_pin_cmd $end_pin
}

proc check_parasitics { } {
  if { ![have_estimated_parasitics] } {
    utl::warn RSZ 21 "no estimated parasitics. Using wire load models."
  }
}

proc parse_slack_margin_arg { keys_var } {
  upvar 1 $keys_var keys
  set slack_margin 0.0
  if { [info exists keys(-slack_margin)] } {
    set slack_margin $keys(-slack_margin)
    sta::check_positive_float "-slack_margin" $slack_margin
    set slack_margin [sta::time_ui_sta $slack_margin]
  }
  return $slack_margin
}

proc parse_max_util { keys_var } {
  upvar 1 $keys_var keys
  set max_util 0.0
  if { [info exists keys(-max_utilization)] } {
    set max_util $keys(-max_utilization)
    if {!([string is double $max_util] && $max_util >= 0.0 && $max_util <= 100)} {
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

proc check_max_wire_length { max_wire_length } {
  if { [rsz::wire_resistance] > 0 } {
    set min_delay_max_wire_length [rsz::find_max_wire_length]
    if { $max_wire_length > 0 } {
      if { $max_wire_length < $min_delay_max_wire_length } {
        utl::warn RSZ 17 "max wire length less than [format %.0fu [sta::distance_sta_ui $min_delay_max_wire_length]] increases wire delays."
      }
    } else {
      # Must follow preamble so buffers are known.
      set max_wire_length $min_delay_max_wire_length
      utl::info RSZ 58 "Using max wire length [format %.0f [sta::distance_sta_ui $max_wire_length]]um."
    }
  }
  return $max_wire_length
}

proc layer_wire_rc { layer_name } {
  set layer [[ord::get_db_tech] findLayer $layer_name]
  if { $layer == "NULL" } {
    utl::error RSZ 2 "layer $layer_name not found."
  }
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

# namespace
}

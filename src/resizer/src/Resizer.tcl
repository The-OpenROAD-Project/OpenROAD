# Resizer, LEF/DEF gate resizer
# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

namespace eval sta {

# Defined by SWIG interface Resizer.i.
define_cmd_args "set_dont_use" {cell dont_use}

define_cmd_args "set_wire_rc" {[-layer layer_name]\
				 [-resistance res ][-capacitance cap]\
				 [-corner corner_name]}

proc set_wire_rc { args } {
   parse_key_args "set_wire_rc" args \
    keys {-layer -resistance -capacitance -corner} flags {}

  set wire_res 0.0
  set wire_cap 0.0

  if { [info exists keys(-layer)] } {
    if { [info exists keys(-resistance)] \
	   || [info exists keys(-capacitance)] } {
      sta_error "Use -layer or -resistance/-capacitance but not both."
    }
    set layer_name $keys(-layer)
    set layer [[[ord::get_db] getTech] findLayer $layer_name]
    if { $layer == "NULL" } {
      sta_error "layer $layer_name not found."
    }
    set layer_width [ord::dbu_to_microns [$layer getWidth]]
    set res_ohm_per_micron [expr [$layer getResistance] / $layer_width]
    set cap_pf_per_micron [expr [ord::dbu_to_microns 1] * $layer_width \
			     * [$layer getCapacitance] \
			     + [$layer getEdgeCapacitance] * 2]
    # ohms/sq
    set wire_res [expr $res_ohm_per_micron * 1e+6]
    # F/m^2
    set wire_cap [expr $cap_pf_per_micron * 1e-12 * 1e+6]
  } else {
    ord::ensure_units_initialized
    if { [info exists keys(-resistance)] } {
      set res $keys(-resistance)
      check_positive_float "-resistance" $res
      set wire_res [expr [resistance_ui_sta $res] / [distance_ui_sta 1.0]]
    }

    if { [info exists keys(-capacitance)] } {
      set cap $keys(-capacitance)
      check_positive_float "-capacitance" $cap
      set wire_cap [expr [capacitance_ui_sta $cap] / [distance_ui_sta 1.0]]
    }
  }

  set corner [parse_corner keys]
  check_argc_eq0 "set_wire_rc" $args

  set_wire_rc_cmd $wire_res $wire_cap $corner
}

define_cmd_args "resize" {[-buffer_inputs]\
			    [-buffer_outputs]\
			    [-resize]\
			    [-repair_max_cap]\
			    [-repair_max_slew]\
			    [-resize_libraries resize_libs]\
			    [-buffer_cell buffer_cell]\
			    [-dont_use lib_cells]\
			    [-max_utilization util]}

proc resize { args } {
  parse_key_args "resize" args \
    keys {-buffer_cell -resize_libraries -dont_use -max_utilization} \
    flags {-buffer_inputs -buffer_outputs -resize -repair_max_cap -repair_max_slew}

  set buffer_inputs [info exists flags(-buffer_inputs)]
  if { $buffer_inputs } {
    ord::warn "resize -buffer_inputs is deprecated. Use the buffer_ports command."
  }
  set buffer_outputs [info exists flags(-buffer_outputs)]
  if { $buffer_outputs } {
    ord::warn "resize -buffer_outputs is deprecated. Use the buffer_ports command."
  }
  set resize [info exists flags(-resize)]
  set repair_max_cap [info exists flags(-repair_max_cap)]
  if { $repair_max_cap } {
    ord::warn "resize -repair_max_cap is deprecated. Use the repair_max_cap command."
  }
  set repair_max_slew [info exists flags(-repair_max_slew)]
  if { $repair_max_slew } {
    ord::warn "resize -repair_max_slew is deprecated. Use the repair_max_slew command."
  }
  # With no options you get the whole salmai.
  if { !($buffer_inputs || $buffer_outputs || $resize \
	   || $repair_max_cap || $repair_max_slew) } {
    set buffer_inputs 1
    set buffer_outputs 1
    set resize 1
    set repair_max_cap 1
    set repair_max_slew 1
  }
  set buffer_cell [parse_buffer_cell keys [expr $buffer_inputs || $buffer_outputs \
					     || $repair_max_cap || $repair_max_slew]]
  if { [info exists keys(-resize_libraries)] } {
    set resize_libs [get_liberty_error "-resize_libraries" $keys(-resize_libraries)]
  } else {
    set resize_libs [get_libs *]
  }

  set dont_use {}
  if { [info exists keys(-dont_use)] } {
    set dont_use [get_lib_cells -quiet $keys(-dont_use)]
  }
  set_dont_use $dont_use
  set_max_utilization [parse_max_util keys]

  check_argc_eq0 "resize" $args

  resizer_preamble $resize_libs

  if { $buffer_inputs } {
    buffer_inputs $buffer_cell
  }
  if { $buffer_outputs } {
    buffer_outputs $buffer_cell
  }
  if { $resize } {
    resize_to_target_slew
  }
  if { $repair_max_cap || $repair_max_slew } {
    repair_max_slew_cap $repair_max_cap $repair_max_slew $buffer_cell
  }
}

proc parse_max_util { keys_var } {
  upvar 1 $keys_var keys
  set max_util 0.0
  if { [info exists keys(-max_utilization)] } {
    set max_util $keys(-max_utilization)
    if {!([string is double $max_util] && $max_util >= 0.0 && $max_util <= 100)} {
      sta_error "-max_utilization must be between 0 and 100%."
    }
    set max_util [expr $max_util / 100.0]
  }
  return $max_util
}

proc parse_buffer_cell { keys_var required } {
  upvar 1 $keys_var keys
  set buffer_cell "NULL"
  if { [info exists keys(-buffer_cell)] } {
    set buffer_cell_name $keys(-buffer_cell)
    # check for -buffer_cell [get_lib_cell arg] return ""
    if { $buffer_cell_name != "" } {
      set buffer_cell [get_lib_cell_error "-buffer_cell" $buffer_cell_name]
      if { $buffer_cell != "NULL" } {
	if { ![get_property $buffer_cell is_buffer] } {
	  sta_error "[get_name $buffer_cell] is not a buffer."
	}
      }
    }
  } elseif { $required } {
    sta_error "-buffer_cell required for buffer insertion."
  }
  if { $buffer_cell == "NULL" && $required } {
    sta_error "-buffer_cell required for buffer insertion."    
  }
  return $buffer_cell
}

define_cmd_args "buffer_ports" {[-inputs] [-outputs]\
				  -buffer_cell buffer_cell\
				  [-max_utilization util]}

proc buffer_ports { args } {
  parse_key_args "buffer_ports" args \
    keys {-buffer_cell -max_utilization} \
    flags {-inputs -outputs}

  set buffer_inputs [info exists flags(-inputs)]
  set buffer_outputs [info exists flags(-outputs)]
  if { !$buffer_inputs && !$buffer_outputs } {
    set buffer_inputs 1
    set buffer_outputs 1
  }
  set buffer_cell [parse_buffer_cell keys 1]

  check_argc_eq0 "buffer_ports" $args

  set_max_utilization [parse_max_util keys]
  if { $buffer_inputs } {
    buffer_inputs $buffer_cell
  }
  if { $buffer_outputs } {
    buffer_outputs $buffer_cell
  }
}

define_cmd_args "repair_max_cap" {-buffer_cell buffer_cell\
				    [-max_utilization util]}

proc repair_max_cap { args } {
  repair_max_cap_slew "repair_max_cap" $args 1 0
}

define_cmd_args "repair_max_slew" {-buffer_cell buffer_cell\
				    [-max_utilization util]}

proc repair_max_slew { args } {
  repair_max_cap_slew "repair_max_slew" $args 0 1
}


proc repair_max_cap_slew { cmd cmd_args repair_max_cap repair_max_slew } {
  parse_key_args "repair_max_slew" cmd_args \
    keys {-buffer_cell -max_utilization} \
    flags {}

  set buffer_cell [parse_buffer_cell keys 1]
  set_max_utilization [parse_max_util keys]

  check_argc_eq0 $cmd $cmd_args

  # init target slews
  resizer_preamble [get_libs *]
  repair_max_slew_cap $repair_max_cap $repair_max_slew $buffer_cell
}

define_cmd_args "repair_max_fanout" {-max_fanout fanout\
				       -buffer_cell buffer_cell\
				       [-max_utilization util]}

proc repair_max_fanout { args } {
  parse_key_args "repair_max_fanout" args \
    keys {-max_fanout -buffer_cell -max_utilization} \
    flags {}

  if { [info exists keys(-max_fanout)] } {
    set max_fanout $keys(-max_fanout)
    check_positive_integer "-max_fanout" $max_fanout
  } else {
    sta_error "no -max_fanout specified."
  }

  set buffer_cell [parse_buffer_cell keys 1]
  set_max_utilization [parse_max_util keys]

  check_argc_eq0 "repair_max_fanout" $args

  repair_max_fanout_cmd $max_fanout $buffer_cell
}

define_cmd_args "repair_hold_violations" {-buffer_cell buffer_cell\
					    [-max_utilization util]}

proc repair_hold_violations { args } {
  parse_key_args "repair_hold_violations" args \
    keys {-buffer_cell -max_utilization} \
    flags {}

  set buffer_cell [parse_buffer_cell keys 1]
  set_max_utilization [parse_max_util keys]

  check_argc_eq0 "repair_hold_violations" $args

  repair_hold_violations_cmd $buffer_cell
}

define_cmd_args "report_design_area" {}

proc report_design_area {} {
  set util [format %.0f [expr [utilization] * 100]]
  set area [format_area [design_area] 0]
  puts "Design area ${area} u^2 ${util}% utilization."
}

define_cmd_args "report_floating_nets" {[-verbose]}

proc report_floating_nets { args } {
  parse_key_args "report_floating_nets" args keys {} flags {-verbose}

  set verbose [info exists flags(-verbose)]
  set floating_nets [find_floating_nets]
  set floating_net_count [llength $floating_nets]
  if { $floating_net_count > 0 } {
    ord::warn "found $floating_net_count floatiing nets."
    if { $verbose } {
      foreach net $floating_nets {
	puts " [get_full_name $net]"
      }
    }
  }
}

define_cmd_args "repair_tie_fanout" {lib_port [-max_fanout fanout] [-verbose]}

proc repair_tie_fanout { args } {
  parse_key_args "repair_tie_fanout" args keys {-max_fanout} flags {-verbose}

  if { [info exists keys(-max_fanout)] } {
    set max_fanout $keys(-max_fanout)
    check_positive_integer "-max_fanout" $max_fanout
  } else {
    sta_error("-max_fanout requried.")
  }
  set verbose [info exists flags(-verbose)]
  
  check_argc_eq1 "repair_tie_fanout" $args
  set lib_port [lindex $args 0]
  if { ![is_object $lib_port] } {
    set lib_port [get_lib_pins [lindex $args 0]]
  }
  if { $lib_port != "NULL" } {
    repair_tie_fanout_cmd $lib_port $max_fanout $verbose
  }
}

# sta namespace end
}

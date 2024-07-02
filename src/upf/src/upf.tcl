############################################################################
##
## Copyright (c) 2022, The Regents of the University of California
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

sta::define_cmd_args "read_upf" { [-file file] }
proc read_upf { args } {
  sta::parse_key_args "read_upf" args \
    keys {-file} flags {}

  source $keys(-file)
}

sta::define_cmd_args "write_upf" {file}
proc write_upf { args } {
  upf::check_block_exists
  sta::check_argc_eq1 "write_upf" $args
  sta::parse_key_args "write_upf" args keys {} flags {}

  upf::write_upf_cmd [lindex $args 0]
}

# Creates a power domain
#
# Arguments:
#
# - elements: list of module paths that belong to this domain OR '*' for top domain
# - name: domain name
sta::define_cmd_args "create_power_domain" { [-elements elements] name }
proc create_power_domain { args } {
  upf::check_block_exists

  sta::parse_key_args "create_power_domain" args \
    keys {-elements} flags {}

  sta::check_argc_eq1 "create_power_domain" $args

  set domain_name [lindex $args 0]
  set elements {}

  if { [info exists keys(-elements)] } {
    set elements $keys(-elements)
  }

  upf::create_power_domain_cmd $domain_name
  foreach {el} $elements {
    upf::update_power_domain_cmd $domain_name $el
  }
}

# Create a logic port to be used within defined domains
#
# Arguments:
#
# - direction: direction of the port (in | out | inout)
# - port_name: port name
sta::define_cmd_args "create_logic_port" { [-direction direction] port_name }
proc create_logic_port { args } {
  upf::check_block_exists

  sta::parse_key_args "create_logic_port" args \
    keys {-direction} flags {}

  sta::check_argc_eq1 "create_logic_port" $args

  set port_name [lindex $args 0]
  set direction ""

  if { [info exists keys(-direction)] } {
    set direction $keys(-direction)
  }

  upf::create_logic_port_cmd $port_name $direction
}

# Creates a power switch
#
# Arguments:
#
# - domain: power domain
# - output_supply_port: The output supply port of the switch
# - input_supply_port: The input supply port of the switch
# - control_port: A control port on the switch
# - on_state {state_name input_supply_port {boolean_expression}}
# - name: power switch name
sta::define_cmd_args "create_power_switch" { \
    [-domain domain] \
    [-output_supply_port output_supply_port] \
    [-input_supply_port input_supply_port] \
    [-control_port control_port] \
    [-on_state on_state] \
    name
}; # checker off
proc create_power_switch { args } {
  upf::check_block_exists

  ord::parse_list_args "create_power_switch" args \
    list {-input_supply_port -control_port -ack_port -on_state}
  sta::parse_key_args "create_power_switch" args \
    keys {-domain -output_supply_port} flags {}; # checker off

  sta::check_argc_eq1 "create_power_switch" $args

  set name [lindex $args 0]
  set domain ""

  if { [info exists keys(-domain)] } {
    set domain $keys(-domain)
  }

  upf::create_power_switch_cmd $name $domain

  if { [info exists keys(-output_supply_port)] } {
    lassign [upf::process_list_arg $keys(-output_supply_port) 2] port_name supply_net_name
    upf::update_power_switch_output_cmd $name $port_name $supply_net_name
  }

  foreach input_port_arg $list(-input_supply_port) {
    lassign [upf::process_list_arg $input_port_arg 2] port_name supply_net_name
    upf::update_power_switch_input_cmd $name $port_name $supply_net_name
  }

  foreach control_port_arg $list(-control_port) {
    lassign [upf::process_list_arg $control_port_arg 2] port_name net_name
    upf::update_power_switch_control_cmd $name $port_name $net_name
  }

  foreach ack_port_arg $list(-ack_port) {
    lassign [upf::process_list_arg $ack_port_arg 3] port_name net_name boolean_expression
    upf::update_power_switch_ack_cmd $name $port_name $net_name $boolean_expression
  }

  foreach on_state_arg $list(-on_state) {
    lassign [upf::process_list_arg $on_state_arg 3] state_name input_supply_port boolean_expression
    upf::update_power_switch_on_cmd $name $state_name $input_supply_port $boolean_expression
  }
}

# Creates/Updates an isolation strategy
#
# Arguments:
#
# - domain: power domain
# - applies_to <inputs|outputs|both>: restricts the strategy to apply only to these
# - clamp_value <0 | 1>: The value the isolation can drive
# - isolation_signal: The control signal for this strategy
# - isolation_sense: The active level of isolation control signal
# - location <parent|self|fanout> : domain in which isolation cells are placed
# - update: flags that the strategy already exists, errors if it doesn't exist
# - name: isolation strategy name

sta::define_cmd_args "set_isolation" { \
    [-domain domain] \
    [-applies_to applies_to] \
    [-clamp_value clamp_value] \
    [-isolation_signal isolation_signal] \
    [-isolation_sense isolation_sense] \
    [-location location] \
    [-update] \
    name
}
proc set_isolation { args } {
  upf::check_block_exists

  sta::parse_key_args "set_isolation" args \
    keys {-domain -applies_to -clamp_value -isolation_signal
          -isolation_sense -location} \
    flags {-update}

  sta::check_argc_eq1 "set_isolation" $args

  set name [lindex $args 0]
  set domain ""
  set applies_to ""
  set clamp_value ""
  set isolation_signal ""
  set isolation_sense ""
  set location ""
  set update 0

  if { [info exists keys(-domain)] } {
    set domain $keys(-domain)
  }

  if { [info exists keys(-applies_to)] } {
    set applies_to $keys(-applies_to)
  }

  if { [info exists keys(-clamp_value)] } {
    set clamp_value $keys(-clamp_value)
  }

  if { [info exists keys(-isolation_signal)] } {
    set isolation_signal $keys(-isolation_signal)
  }

  if { [info exists keys(-isolation_sense)] } {
    set isolation_sense $keys(-isolation_sense)
  }

  if { [info exists keys(-location)] } {
    set location $keys(-location)
  }

  if { [info exists flags(-update)] } {
    set update 1
  }

  upf::set_isolation_cmd $name $domain $update $applies_to $clamp_value \
    $isolation_signal $isolation_sense $location

}

# Specifies the cells to be used for an isolation strategy
#
# Arguments:
#
# - domain: power domain
# - strategy: isolation strategy name
# - lib_cells: list of lib cells that could be used

sta::define_cmd_args "use_interface_cell" { \
    [-domain domain] \
    [-strategy strategy] \
    [-lib_cells lib_cells]
}
proc use_interface_cell { args } {
  upf::check_block_exists

  sta::parse_key_args "use_interface_cell" args \
    keys {-domain -strategy -lib_cells} flags {}

  sta::check_argc_eq0 "use_interface_cell" $args

  set domain ""
  set strategy ""
  set lib_cells {}

  if { [info exists keys(-domain)] } {
    set domain $keys(-domain)
  }

  if { [info exists keys(-strategy)] } {
    set strategy $keys(-strategy)
  }

  if { [info exists keys(-lib_cells)] } {
    set lib_cells $keys(-lib_cells)
  }

  foreach {cell} $lib_cells {
    upf::use_interface_cell_cmd $domain $strategy $cell
  }
}


# Specifies the area that should be occupied by a given domain
# example: set_domain_area PD_D1 -area {27 27 60 60}
#
# Argument list:
#
# - domain_name: power domain name
# - area: a list of 4 coordinates (lower left x, lower left y, upper right x, upper right y)
sta::define_cmd_args "set_domain_area" {domain_name -area {llx lly urx ury}}

proc set_domain_area { args } {
  upf::check_block_exists

  sta::parse_key_args "set_domain_area" args keys {-area} flags {}
  set domain_name [lindex $args 0]
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error UPF 36 "-area is a list of 4 coordinates"
    }
    lassign $area lx ly ux uy
    sta::check_positive_float "-area" $lx
    sta::check_positive_float "-area" $ly
    sta::check_positive_float "-area" $ux
    sta::check_positive_float "-area" $uy
  } else {
    utl::error UPF 37 "please define area"
  }
  sta::check_argc_eq1 "set_domain_area" $args
  set domain_name $args

  set lx [ord::microns_to_dbu $lx]
  set ly [ord::microns_to_dbu $ly]
  set ux [ord::microns_to_dbu $ux]
  set uy [ord::microns_to_dbu $uy]
  set area [odb::new_Rect $lx $ly $ux $uy]
  upf::set_domain_area_cmd $domain_name $area
}

# Specify which power-switch model is to be used for the implementation of the corresponding switch
# instance
#
#
# Argument list:
#  - switch_name_list []: A list of switches (as defined by create_power_switch) to map.
#  - lib_cells [] : A list of library cells.
#  - port_map {{mapped_model_port switch_port_or_supply_net_ref} *} :
#                         mapped_model_ port is a port on the model being mapped.
#                         switch_ port_or_supply_net_ref indicates a supply or logic port on a
#                         switch: an input supply port, output supply port, control port, or
#                         acknowledge port; or it references a supply net from a supply set
#                         associated with the switch.

sta::define_cmd_args "map_power_switch" { \
    [-switch_name_list switch_name_list] \
    [-lib_cells lib_cells] \
    [-port_map port_map]
}

proc map_power_switch { args } {
  upf::check_block_exists

  sta::parse_key_args "map_power_switch" args \
    keys {-switch_name_list -lib_cells -port_map} flags {}

  sta::check_argc_eq1 "map_power_switch" $args

  set switch_name_list [lindex $args 0]
  set lib_cells {}
  set port_map {}

  if { [info exists keys(-lib_cells)] } {
    set lib_cells $keys(-lib_cells)
  }

  if { [info exists keys(-port_map)] } {
    set port_map $keys(-port_map)
  }


  for { set i 0 } { $i < [llength $switch_name_list] } { incr i } {
    set switch [lindex $switch_name_list $i]
    set cell [lindex $lib_cells $i]
    upf::set_power_switch_cell $switch $cell

    foreach {port} $port_map {
      if {[llength $port] != 2} {
        utl::error UPF 40 "The port map should be a list of exactly 2 elements"
      }
      upf::set_power_switch_port $switch [lindex $port 0] [lindex $port 1]
    }
  }
}

# Define command arguments for set_level_shifter
sta::define_cmd_args "set_level_shifter" { \
    [-domain domain] \
    [-elements elements] \
    [-exclude_elements exclude_elements] \
    [-source source] \
    [-sink sink] \
    [-use_functional_equivalence use_functional_equivalence] \
    [-applies_to applies_to] \
    [-applies_to_boundary applies_to_boundary] \
    [-rule rule] \
    [-threshold threshold] \
    [-no_shift] \
    [-force_shift] \
    [-location location] \
    [-input_supply input_supply] \
    [-output_supply output_supply] \
    [-internal_supply internal_supply] \
    [-name_prefix name_prefix] \
    [-name_suffix name_suffix] \
    [-instance instance] \
    [-update] \
    [-use_equivalence use_equivalence] \
    name
}

# Procedure to set or update a level shifter
proc set_level_shifter { args } {
  upf::check_block_exists

  sta::parse_key_args "set_level_shifter" args \
    keys {-domain -elements -exclude_elements -source -sink \
          -use_functional_equivalence -applies_to -applies_to_boundary \
          -rule -threshold  -location -input_supply -output_supply \
          -internal_supply -name_prefix -name_suffix -instance \
          -use_equivalence} \
    flags {-update -no_shift -force_shift}

  sta::check_argc_eq1 "set_level_shifter" $args

  set name [lindex $args 0]

  set domain ""
  set elements {}
  set exclude_elements {}
  set source ""
  set sink ""
  set use_functional_equivalence "TRUE"
  set applies_to ""
  set applies_to_boundary ""
  set rule ""
  set threshold ""
  set no_shift ""
  set force_shift ""
  set location ""
  set input_supply ""
  set output_supply ""
  set internal_supply ""
  set name_prefix ""
  set name_suffix ""
  set instance {}
  set update 0

  if { [info exists keys(-domain)] } {
    set domain $keys(-domain)
  }

  if { [info exists keys(-elements)] } {
    set elements $keys(-elements)
  }

  if { [info exists keys(-exclude_elements)] } {
    set exclude_elements $keys(-exclude_elements)
  }

  if { [info exists keys(-source)] } {
    set source $keys(-source)
  }

  if { [info exists keys(-sink)] } {
    set sink $keys(-sink)
  }

  if { [info exists keys(-use_functional_equivalence)] } {
    set use_functional_equivalence $keys(-use_functional_equivalence)
  }

  if { [info exists keys(-applies_to)] } {
    set applies_to $keys(-applies_to)
  }

  if { [info exists keys(-applies_to_boundary)] } {
    set applies_to_boundary $keys(-applies_to_boundary)
  }

  if { [info exists keys(-rule)] } {
    set rule $keys(-rule)
  }

  if { [info exists keys(-threshold)] } {
    set threshold $keys(-threshold)
  }

  if { [info exists keys(-location)] } {
    set location $keys(-location)
  }

  if { [info exists keys(-input_supply)] } {
    set input_supply $keys(-input_supply)
  }

  if { [info exists keys(-output_supply)] } {
    set output_supply $keys(-output_supply)
  }

  if { [info exists keys(-internal_supply)] } {
    set internal_supply $keys(-internal_supply)
  }

  if { [info exists keys(-name_prefix)] } {
    set name_prefix $keys(-name_prefix)
  }

  if { [info exists keys(-name_suffix)] } {
    set name_suffix $keys(-name_suffix)
  }

  if { [info exists keys(-instance)] } {
    set instance $keys(-instance)
  }

  if { [info exists keys(-use_equivalence)] } {
    utl::warn UPF 57 "-use_equivalence is deprecated in UPF and not supported in OpenROAD"
  }

  if { [info exists flags(-update)] } {
    set update 1
  }

  if { [info exists flags(-no_shift)] } {
    set no_shift "1"
  }

  if { [info exists flags(-force_shift)] } {
    set force_shift "1"
  }

  set ok [upf::create_or_update_level_shifter_cmd $name $domain $source \
          $sink $use_functional_equivalence $applies_to $applies_to_boundary \
          $rule $threshold $no_shift $force_shift $location $input_supply \
          $output_supply $internal_supply $name_prefix $name_suffix $update]

  if { $ok == 0 } {
    return
  }

  foreach element $elements {
    upf::add_level_shifter_element_cmd $name $element
  }

  foreach exclude_element $exclude_elements {
    upf::exclude_level_shifter_element_cmd $name $exclude_element
  }

  foreach inst_port $instance {
    lassign $inst_port instance_name port_name
    upf::handle_level_shifter_instance_cmd $name $instance_name $port_name
  }
}


# Procedure to set the voltage of a power domain which takes a float voltage
# Arguments: -domain, -voltage

sta::define_cmd_args "set_domain_voltage" { \
    [-domain domain] \
    [-voltage voltage]
}

proc set_domain_voltage { args } {
  upf::check_block_exists

  sta::parse_key_args "set_domain_voltage" args \
    keys {-domain -voltage} flags {}

  sta::check_argc_eq0 "set_domain_voltage" $args

  set domain ""
  set voltage 0.0

  if { [info exists keys(-domain)] } {
    set domain $keys(-domain)
  }

  if { [info exists keys(-voltage)] } {
    set voltage $keys(-voltage)
  }

  upf::set_domain_voltage_cmd $domain $voltage
}

# Procedure to set the library cell used for level shifter identifying
# cell_name, input_port, output_port for a given level_shifter strategy

# Arguments: -level_shifter, -cell_name, -input_port, -output_port

sta::define_cmd_args "set_level_shifter_cell" { \
    [-level_shifter level_shifter] \
    [-cell_name cell_name] \
    [-input_port input_port] \
    [-output_port output_port]
}

proc set_level_shifter_cell { args } {
  upf::check_block_exists

  sta::parse_key_args "set_level_shifter_cell" args \
    keys {-level_shifter -cell_name -input_port -output_port} flags {}

  sta::check_argc_eq0 "set_level_shifter_cell" $args

  set level_shifter ""
  set cell_name ""
  set input_port ""
  set output_port ""

  if { [info exists keys(-level_shifter)] } {
    set level_shifter $keys(-level_shifter)
  }

  if { [info exists keys(-cell_name)] } {
    set cell_name $keys(-cell_name)
  }

  if { [info exists keys(-input_port)] } {
    set input_port $keys(-input_port)
  }

  if { [info exists keys(-output_port)] } {
    set output_port $keys(-output_port)
  }

  upf::set_level_shifter_cell_cmd $level_shifter $cell_name $input_port $output_port
}

namespace eval upf {
proc process_list_arg { args max_len } {
  while { [llength $args] < $max_len } {
    lappend args ""
  }
  return $args
}

## check if a chip/block has been created and exits with error if not
proc check_block_exists { } {
  set db [ord::get_db]
  set chip [$db getChip]

  if { "$chip" eq "NULL" } {
    utl::error UPF 33 "No Chip exists"
  }

  set block [$chip getBlock]

  if { "$block" eq "NULL" } {
    utl::error UPF 34 "No block exists"
  }
}

}

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

# Creates a power domain
#
# Arguments:
#
# - elements: list of module paths that belong to this domain OR '*' for top domain
# - name: domain name
sta::define_cmd_args "create_power_domain" { [-elements elements] name }
proc create_power_domain { args } {
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
}
proc create_power_switch { args } {
    sta::parse_key_args "create_power_switch" args \
        keys {-domain -output_supply_port -input_supply_port -control_port -on_state} flags {}

    sta::check_argc_eq1 "create_power_switch" $args

    set name [lindex $args 0]
    set domain ""
    set output_supply_port ""
    set input_supply_port ""
    set control_port {}
    set on_state {}

    if { [info exists keys(-domain)] } {
        set domain $keys(-domain)
    }

    if { [info exists keys(-output_supply_port)] } {
        set output_supply_port $keys(-output_supply_port)
    }

    if { [info exists keys(-input_supply_port)] } {
        set input_supply_port $keys(-input_supply_port)
    }

    if { [info exists keys(-control_port)] } {
        set control_port $keys(-control_port)
    }

    if { [info exists keys(-on_state)] } {
        set on_state $keys(-on_state)
    }

    upf::create_power_switch_cmd $name $domain $output_supply_port $input_supply_port

    foreach {control} $control_port {
        upf::update_power_switch_control_cmd $name $control
    }

    foreach {on} $on_state {
        upf::update_power_switch_on_cmd $name $on
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
    sta::parse_key_args "set_isolation" args \
        keys {-domain -applies_to -clamp_value -isolation_signal -isolation_sense -location} flags {-update}

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

    upf::set_isolation_cmd $name $domain $update $applies_to $clamp_value $isolation_signal $isolation_sense $location

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
  sta::parse_key_args "set_domain_area" args keys {-area} flags {}
  set domain_name [lindex $args 0]
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error ODB 9315 "-area is a list of 4 coordinates"
    }
    lassign $area llx lly urx ury
    sta::check_positive_float "-area" $llx
    sta::check_positive_float "-area" $lly
    sta::check_positive_float "-area" $urx
    sta::check_positive_float "-area" $ury
  } else {
    utl::error ODB 9316 "please define area"
  }
  sta::check_argc_eq1 "set_domain_area" $args
  set domain_name $args
  
  upf::set_domain_area_cmd $domain_name $llx $lly $urx $ury
}




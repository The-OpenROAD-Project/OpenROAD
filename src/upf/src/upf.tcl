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

# Creates a power domain
#
# Arguments:
#
# - elements: list of module paths that belong to this domain OR '*' for top domain
# - name: domain name
sta::define_cmd_args "create_power_domain" { [-elements elements] name }
proc create_power_domain { args } {
    check_block_exists

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
    check_block_exists

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
    check_block_exists

    sta::parse_key_args "create_power_switch" args \
        keys {-domain -output_supply_port -input_supply_port -control_port -on_state} flags {}

    sta::check_argc_eq1 "create_power_switch" $args

    set name [lindex $args 0]
    set domain ""
    set output_supply_port {}
    set input_supply_port {}
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

    upf::create_power_switch_cmd $name $domain

    foreach {input} $input_supply_port {
        upf::update_power_switch_input_cmd $name $input
    }

    foreach {output} $output_supply_port {
        upf::update_power_switch_output_cmd $name $output
    }

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
    check_block_exists

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
    check_block_exists

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
    check_block_exists

    sta::parse_key_args "set_domain_area" args keys {-area} flags {}
    set domain_name [lindex $args 0]
    if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
        utl::error UPF 36 "-area is a list of 4 coordinates"
    }
    lassign $area llx lly urx ury
    sta::check_positive_float "-area" $llx
    sta::check_positive_float "-area" $lly
    sta::check_positive_float "-area" $urx
    sta::check_positive_float "-area" $ury
    } else {
    utl::error UPF 37 "please define area"
    }
    sta::check_argc_eq1 "set_domain_area" $args
    set domain_name $args

    upf::set_domain_area_cmd $domain_name $llx $lly $urx $ury
}

# Specify which power-switch model is to be used for the implementation of the corresponding switch
# instance
#
#
# Argument list: 
#  - switch_name_list []: A list of switches (as defined by create_power_switch) to map.
#  - lib_cells [] : A list of library cells.
#  - port_map {{mapped_model_port switch_port_or_supply_net_ref} *} : mapped_model_ port is a port on the model being mapped.
#                                                                     switch_ port_or_supply_net_ref indicates a supply or logic port on a
#                                                                     switch: an input supply port, output supply port, control port, or
#                                                                     acknowledge port; or it references a supply net from a supply set
#                                                                     associated with the switch.

sta::define_cmd_args "map_power_switch" { \
    [-switch_name_list switch_name_list] \
    [-lib_cells lib_cells] \
    [-port_map port_map] 
}

proc map_power_switch { args } {
    check_block_exists

    sta::parse_key_args "map_power_switch" args \
        keys {switch_name_list -lib_cells -port_map} flags {}

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
    check_block_exists

    sta::parse_key_args "set_level_shifter" args \
        keys {-domain -elements -exclude_elements -source -sink -use_functional_equivalence -applies_to -applies_to_boundary -rule -threshold  -location -input_supply -output_supply -internal_supply -name_prefix -name_suffix -instance  -use_equivalence} flags {-update -no_shift -force_shift}

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

    set ok [upf::create_or_update_level_shifter_cmd $name $domain $source $sink $use_functional_equivalence $applies_to $applies_to_boundary $rule $threshold $no_shift $force_shift $location $input_supply $output_supply $internal_supply $name_prefix $name_suffix $update]

    if { $ok == 0 } {
        return
    }

    foreach element $elements {
        upf::add_level_shifter_element_cmd $name $element
    }

    foreach exclude_element $exclude_elements {
        upf::exclude_level_shifter_element_cmd $name $exclude_element
    }

    foreach {instance_name port_name} $instance {
        upf::handle_level_shifter_instance_cmd $name $instance_name $port_name
    }

}

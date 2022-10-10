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





# BSD 3-Clause License
#
# Copyright (c) 2020, MICL, DD-Lab, University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.



sta::define_cmd_args "check_antenna" { [-verbose] }
sta::define_cmd_args "check_par_max_length" { [-net_name netname]\
                                              [-route_level rt_lv] }

# Put helper functions in a separate namespace so they are not visible
# too users in the global namespace.
namespace eval antennachecker {

proc antennachecker_helper { } {
  puts "This is an Antenna Checker for OpenRoad. Version: 05/04/2020"
  puts "checking work flow"
  # antennachecker::
}

}

proc check_antenna { args } {
  sta::parse_key_args "check_antenna" args \
  keys {} \
  flags {-verbose}

  antennachecker::antennachecker_set_verbose [info exists flags(-verbose)]
  antennachecker::antennachecker_helper
  antennachecker::check_antenna
}

proc check_par_max_length { args } {
  sta::parse_key_args "check_par_max_length" args \
    keys {-net_name -route_level} \
    flags {}

  if { [info exists keys(-net_name)] } {
    set netname $keys(-net_name)
    antennachecker::antennachecker_set_net_name $netname

    if { [info exists keys(-route_level)] } {
      set rt_lv $keys(-route_level)
      sta::check_positive_integer "-route_level" $rt_lv
      antennachecker::antennachecker_set_route_level $rt_lv
    } else {
      sta::sta_error "no -route_level specified."
    }
  } else {
    sta::sta_error "no -net_name specified."
  }
  antennachecker::antennachecker_helper
  antennachecker::check_par_max_length
}



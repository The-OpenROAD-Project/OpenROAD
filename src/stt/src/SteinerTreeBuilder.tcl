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

sta::define_cmd_args "set_routing_alpha" { alpha \
                                          [-net net_name] }

proc set_routing_alpha { args } {
  sta::parse_key_args "set_routing_alpha" args \
                 keys {-net}

  set alpha [lindex $args 0]
  if { ![string is double $alpha] || $alpha < 0.0 || $alpha > 1.0 } {
    utl::error STT 1 "The alpha value must be between 0.0 and 1.0."
  }
  if { [info exists keys(-net)] } {
    set net_names $keys(-net)
    set nets [stt::parse_net_names "set_routing_alpha" $net_names]
    foreach net $nets {
      stt::set_alpha_for_net $net $alpha
    }
  } elseif { [llength $args] == 1 } {
    stt::set_routing_alpha_cmd $alpha
  } else {
    utl::error STT 2 "set_routing_alpha: Wrong number of arguments."
  }
}

namespace eval stt {

proc parse_net_names {cmd names} {
  set dbBlock [ord::get_db_block]
  set net_list {}
  foreach net [get_nets $names] {
    lappend net_list [sta::sta_to_db_net $net]
  }

  if {[llength $net_list] == 0} {
    utl::error GRT 102 "Nets for $cmd command were not found"
  }

  return $net_list
}

# stt namespace end
}
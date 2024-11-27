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
                                          [-net net_name] \
                                          [-min_fanout fanout] \
                                          [-min_hpwl hpwl] \
                                          [-clock_nets]
} ;# checker off

proc set_routing_alpha { args } {
  ord::parse_list_args "set_routing_alpha" args list {-net}
  sta::parse_key_args "set_routing_alpha" args \
    keys {-min_fanout -min_hpwl} \
    flags {-clock_nets} ;# checker off

  sta::check_argc_eq1 "set_routing_alpha" $args

  set alpha [lindex $args 0]
  if { ![string is double $alpha] || $alpha < 0.0 || $alpha > 1.0 } {
    utl::error STT 1 "The alpha value must be between 0.0 and 1.0."
  }

  if { [llength $list(-net)] > 0 } {
    foreach net $list(-net) {
      stt::set_net_alpha [stt::find_net $net] $alpha
    }
  } elseif { [info exists keys(-min_fanout)] } {
    set fanout $keys(-min_fanout)
    stt::set_min_fanout_alpha $fanout $alpha
  } elseif { [info exists keys(-min_hpwl)] } {
    set hpwl [ord::microns_to_dbu $keys(-min_hpwl)]
    stt::set_min_hpwl_alpha $hpwl $alpha
  } elseif { [info exists flags(-clock_nets)] } {
    set nets [stt::filter_clk_nets "set_routing_alpha"]
    foreach net $nets {
      stt::set_net_alpha $net $alpha
    }
  } else {
    stt::set_routing_alpha_cmd $alpha
  }
}

namespace eval stt {
proc find_net { name } {
  return [sta::sta_to_db_net [get_nets $name]]
}

proc filter_clk_nets { cmd } {
  set dbBlock [ord::get_db_block]
  set net_list {}
  foreach net [$dbBlock getNets] {
    if { [$net getSigType] == "CLOCK" } {
      lappend net_list $net
    }
  }

  if { [llength $net_list] == 0 } {
    utl::error STT 6 "Clock nets for $cmd command were not found"
  }

  return $net_list
}

# stt namespace end
}

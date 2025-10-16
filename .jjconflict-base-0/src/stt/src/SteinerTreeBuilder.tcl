# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

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

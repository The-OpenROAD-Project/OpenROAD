# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

sta::define_cmd_args "sv_elaborate" {<elaboration-arguments>}

proc sv_elaborate { args } {
  syn::sv_elaborate_cmd [join $args " "]
}

sta::define_cmd_args "synthesize" {-reduce_name_loss}

proc synthesize { args } {
  sta::parse_key_args "synthesize" args keys {} flags {-reduce_name_loss}

  # bitblast all but arithmetic
  syn::bitblast_cmd false
  syn::liveness_opt_cmd

  if { ![info exists flags(-reduce_name_loss)] } {
    syn::abc_roundtrip {&ps; &st; &fraig -v -w;}
  }

  # now bitblast incl. arithmetic
  syn::bitblast_cmd true
  syn::opt_cmd

  if { ![info exists flags(-reduce_name_loss)] } {
    syn::abc_roundtrip {&ps; &st; &dc2 -v; &dc2 -v; &if -g -K 6; &dc2 -v; &dc2 -v; &dc2 -v; &ps}
  }

  syn::map_sequentials
  syn::map_combinationals
  syn::gate_fuse_opt_cmd
  syn::stats
  syn::export_to_odb
}

namespace eval syn {
proc remove_ports { pattern } {
  remove_ports_cmd $pattern
}

proc parse { filename } {
  parse_cmd $filename
}

proc dump_fanin_cone { args } {
  sta::parse_key_args "dump_fanin_cone" args \
    keys {-depth} \
    flags {-stop_at_stateful}

  set max_depth 10
  if { [info exists keys(-depth)] } {
    set max_depth $keys(-depth)
  }

  set stop_at_stateful [info exists flags(-stop_at_stateful)]

  sta::check_argc_eq1 "dump_fanin_cone" $args
  set net_ref [lindex $args 0]

  dump_fanin_cone_cmd $net_ref $max_depth $stop_at_stateful
}

proc dump_fanout_cone { args } {
  sta::parse_key_args "dump_fanout_cone" args \
    keys {-depth} \
    flags {-stop_at_stateful}

  set max_depth 10
  if { [info exists keys(-depth)] } {
    set max_depth $keys(-depth)
  }

  set stop_at_stateful [info exists flags(-stop_at_stateful)]

  sta::check_argc_eq1 "dump_fanout_cone" $args
  set net_ref [lindex $args 0]

  dump_fanout_cone_cmd $net_ref $max_depth $stop_at_stateful
}

proc acd_resynth { args } {
  sta::parse_key_args "acd_resynth" args \
    keys {-max_leaves -max_intermediate_leaves -max_cells -max_outerfans -effort} \
    flags {-exclude_buffers -allow_lateral -apply}

  set max_leaves 6
  if { [info exists keys(-max_leaves)] } {
    set max_leaves $keys(-max_leaves)
    sta::check_positive_integer "-max_leaves" $max_leaves
  }

  set max_cells 20
  if { [info exists keys(-max_cells)] } {
    set max_cells $keys(-max_cells)
    sta::check_positive_integer "-max_cells" $max_cells
  }

  set max_intermediate_leaves 8
  if { [info exists keys(-max_intermediate_leaves)] } {
    set max_intermediate_leaves $keys(-max_intermediate_leaves)
    sta::check_positive_integer "-max_intermediate_leaves" $max_intermediate_leaves
  }

  set max_outerfans 3
  if { [info exists keys(-max_outerfans)] } {
    set max_outerfans $keys(-max_outerfans)
    sta::check_positive_integer "-max_outerfans" $max_outerfans
  }

  set exclude_buffers [info exists flags(-exclude_buffers)]
  set allow_lateral [info exists flags(-allow_lateral)]


  set timing_opt_effort [expr 1.0e11]
  if { [info exists keys(-effort)] } {
    set timing_opt_effort $keys(-effort)
    sta::check_positive_float "-effort" $timing_opt_effort
  }

  set apply [info exists flags(-apply)]

  sta::check_argc_eq0 "acd_resynth" $args

  acd_resynth_cmd $max_leaves $max_intermediate_leaves $max_cells \
    $max_outerfans $exclude_buffers $allow_lateral $timing_opt_effort \
    $apply
}

proc dump { { filename "" } } {
  dump_cmd $filename
}

proc stats { } {
  stats_cmd
}

proc memory_usage { } {
  memory_usage_cmd
}
}

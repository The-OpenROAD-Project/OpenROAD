# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

sta::define_cmd_args "read_itf" {filename}

proc read_itf { args } {
  sta::parse_key_args "read_itf" args \
    keys {} flags {}
  sta::check_argc_eq1 "read_itf" $args
  fsv::read_itf_cmd [file nativename [lindex $args 0]]
}

sta::define_cmd_args "set_field_solver_permittivity" {permittivity}

proc set_field_solver_permittivity { args } {
  sta::parse_key_args "set_field_solver_permittivity" args \
    keys {} flags {}
  sta::check_argc_eq1 "set_field_solver_permittivity" $args
  set eps [lindex $args 0]
  sta::check_positive_float "permittivity" $eps
  fsv::set_permittivity_cmd $eps
}

sta::define_cmd_args "add_field_solver_box" {-net net\
                                              [-box box]\
                                              [-layer layer]\
                                              [-rect rect]}

proc add_field_solver_box { args } {
  sta::parse_key_args "add_field_solver_box" args \
    keys {-net -box -layer -rect} flags {}
  sta::check_argc_eq0 "add_field_solver_box" $args
  set net [fsv::required_key -net [array get keys]]
  if { [info exists keys(-box)] } {
    if { [info exists keys(-layer)] || [info exists keys(-rect)] } {
      utl::error FSV 21 "-box cannot be combined with -layer or -rect."
    }
    set box [fsv::check_numbers "-box" $keys(-box) 6]
    fsv::add_box_cmd $net {*}$box
  } elseif { [info exists keys(-layer)] && [info exists keys(-rect)] } {
    set rect [fsv::check_numbers "-rect" $keys(-rect) 4]
    fsv::add_layer_rect_cmd $net $keys(-layer) {*}$rect
  } else {
    utl::error FSV 22 "Give -box, or -layer with -rect."
  }
}

sta::define_cmd_args "add_field_solver_rect" {-net net\
                                               [-rect rect]\
                                               [-layer layer]\
                                               [-x span]}

proc add_field_solver_rect { args } {
  sta::parse_key_args "add_field_solver_rect" args \
    keys {-net -rect -layer -x} flags {}
  sta::check_argc_eq0 "add_field_solver_rect" $args
  set net [fsv::required_key -net [array get keys]]
  if { [info exists keys(-rect)] } {
    if { [info exists keys(-layer)] || [info exists keys(-x)] } {
      utl::error FSV 16 "-rect cannot be combined with -layer or -x."
    }
    set rect [fsv::check_numbers "-rect" $keys(-rect) 4]
    fsv::add_rect_cmd $net {*}$rect
  } elseif { [info exists keys(-layer)] && [info exists keys(-x)] } {
    set span [fsv::check_numbers "-x" $keys(-x) 2]
    fsv::add_layer_span_cmd $net $keys(-layer) {*}$span
  } else {
    utl::error FSV 17 "Give -rect, or -layer with -x."
  }
}

sta::define_cmd_args "add_field_solver_nets" {-nets nets\
                                               [-halo halo]\
                                               [-margin margin]\
                                               [-layer_map layer_map]}

proc add_field_solver_nets { args } {
  sta::parse_key_args "add_field_solver_nets" args \
    keys {-nets -halo -margin -layer_map} flags {}
  sta::check_argc_eq0 "add_field_solver_nets" $args
  set nets [fsv::required_key -nets [array get keys]]
  # -1 takes the default, a multiple of the nets' height.
  set halo -1
  set margin -1
  if { [info exists keys(-halo)] } {
    set halo [fsv::check_distance -halo $keys(-halo)]
  }
  if { [info exists keys(-margin)] } {
    set margin [fsv::check_distance -margin $keys(-margin)]
  }
  set layer_map {}
  if { [info exists keys(-layer_map)] } {
    set layer_map $keys(-layer_map)
    if { [llength $layer_map] % 2 != 0 } {
      utl::error FSV 32 "-layer_map needs pairs of design and stack layers."
    }
  }
  fsv::add_nets_cmd $nets $halo $margin $layer_map
}

sta::define_cmd_args "clear_field_solver" {}

proc clear_field_solver { args } {
  sta::parse_key_args "clear_field_solver" args \
    keys {} flags {}
  sta::check_argc_eq0 "clear_field_solver" $args
  fsv::clear_cmd
}

sta::define_cmd_args "solve_capacitance" {[-max_walks walks]\
                                           [-min_walks walks]\
                                           [-nets nets]\
                                           [-panels panels]\
                                           [-seed seed]\
                                           [-tol_coupling tolerance]\
                                           [-tol_total tolerance]}

proc solve_capacitance { args } {
  sta::parse_key_args "solve_capacitance" args \
    keys {-max_walks -min_walks -nets -panels -seed -tol_coupling \
          -tol_total} \
    flags {}
  sta::check_argc_eq0 "solve_capacitance" $args

  # -1 keeps the solver's default.
  set values {}
  foreach {key check} {-tol_total fsv::check_tolerance \
                       -tol_coupling fsv::check_tolerance \
                       -min_walks {fsv::check_count 0} \
                       -max_walks {fsv::check_count 1} \
                       -seed {fsv::check_count 0} \
                       -panels fsv::check_panels} {
    if { [info exists keys($key)] } {
      {*}$check $key $keys($key)
      lappend values $keys($key)
    } else {
      lappend values -1
    }
  }

  if { [info exists keys(-nets)] } {
    set nets $keys(-nets)
  } else {
    set nets [fsv::default_targets_cmd]
  }
  if { [llength $nets] == 0 } {
    utl::error FSV 23 "No nets to solve; add boxes or rectangles first."
  }

  set result [dict create]
  foreach net $nets {
    dict set result $net [fsv::solve_net_cmd $net {*}$values]
  }
  return $result
}

namespace eval fsv {
# The value of a required key, from a parse_key_args array.
proc required_key { key keys_list } {
  array set keys $keys_list
  if { ![info exists keys($key)] } {
    utl::error FSV 20 "$key is required."
  }
  return $keys($key)
}

proc check_distance { key value } {
  if { !([string is double -strict $value] && $value >= 0) } {
    utl::error FSV 33 "$key must be a distance of at least 0."
  }
  return $value
}

proc check_tolerance { key value } {
  if { !([string is double -strict $value] && $value > 0) } {
    utl::error FSV 25 "$key must be a number greater than 0."
  }
}

# An integer from min to the largest 64-bit signed integer.
proc check_count { min key value } {
  if { !([string is wide -strict $value] && $value >= $min) } {
    utl::error FSV 27 "$key must be an integer of at least $min."
  }
}

proc check_panels { key value } {
  if {
    !([string is integer -strict $value] && $value > 0
      && $value % 16 == 0)
  } {
    utl::error FSV 28 "$key must be a positive multiple of 16."
  }
}

# The numbers in a list, which must have the given length.
proc check_numbers { key values count } {
  if { [llength $values] != $count } {
    utl::error FSV 24 "$key needs $count numbers."
  }
  foreach value $values {
    sta::check_float $key $value
  }
  return $values
}
}

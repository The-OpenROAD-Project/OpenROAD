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
  if { ![info exists keys(-net)] } {
    utl::error FSV 20 "-net is required."
  }
  set net $keys(-net)
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

sta::define_cmd_args "clear_field_solver" {}

proc clear_field_solver { args } {
  sta::parse_key_args "clear_field_solver" args \
    keys {} flags {}
  sta::check_argc_eq0 "clear_field_solver" $args
  fsv::clear_cmd
}

sta::define_cmd_args "solve_capacitance" {[-coupling_threshold fraction]\
                                           [-max_walks walks]\
                                           [-min_walks walks]\
                                           [-nets nets]\
                                           [-panels panels]\
                                           [-seed seed]\
                                           [-tol_coupling tolerance]\
                                           [-tol_total tolerance]}

proc solve_capacitance { args } {
  sta::parse_key_args "solve_capacitance" args \
    keys {-coupling_threshold -max_walks -min_walks -nets -panels -seed \
          -tol_coupling -tol_total} \
    flags {}
  sta::check_argc_eq0 "solve_capacitance" $args

  # -1 keeps the solver's default.
  set values {}
  foreach {key check} {-tol_total fsv::check_tolerance \
                       -tol_coupling fsv::check_tolerance \
                       -coupling_threshold fsv::check_fraction \
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
    set nets [fsv::net_names_cmd]
  }
  if { [llength $nets] == 0 } {
    utl::error FSV 23 "No nets to solve; add boxes with add_field_solver_box."
  }

  set result [dict create]
  foreach net $nets {
    dict set result $net [fsv::solve_net_cmd $net {*}$values]
  }
  return $result
}

namespace eval fsv {
proc check_tolerance { key value } {
  if { !([string is double -strict $value] && $value > 0) } {
    utl::error FSV 25 "$key must be a number greater than 0."
  }
}

proc check_fraction { key value } {
  if { !([string is double -strict $value] && $value >= 0 && $value <= 1) } {
    utl::error FSV 26 "$key must be a number from 0 to 1."
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

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

sta::define_cmd_args "build_pex_model" {-itf itf\
                                         -out model\
                                         [-layers layers]\
                                         [-tol tolerance]}

proc build_pex_model { args } {
  sta::parse_key_args "build_pex_model" args \
    keys {-itf -out -layers -tol} flags {}
  sta::check_argc_eq0 "build_pex_model" $args
  set itf [pex::required_key -itf [array get keys]]
  set out [pex::required_key -out [array get keys]]
  set layers {}
  if { [info exists keys(-layers)] } {
    set layers $keys(-layers)
  }
  set tol 0.005
  if { [info exists keys(-tol)] } {
    set tol [pex::check_positive -tol $keys(-tol)]
  }
  pex::build_model_cmd [file nativename $itf] [file nativename $out] \
    $layers $tol
}

sta::define_cmd_args "read_pex_model" {-itf itf model}

proc read_pex_model { args } {
  sta::parse_key_args "read_pex_model" args \
    keys {-itf} flags {}
  sta::check_argc_eq1 "read_pex_model" $args
  set itf [pex::required_key -itf [array get keys]]
  pex::read_model_cmd [file nativename $itf] [file nativename [lindex $args 0]]
}

sta::define_cmd_args "extract_pex" {[-max_res resistance]\
                                     [-coupling_threshold capacitance]\
                                     [-coupling_ratio ratio]}

proc extract_pex { args } {
  sta::parse_key_args "extract_pex" args \
    keys {-max_res -coupling_threshold -coupling_ratio} flags {}
  sta::check_argc_eq0 "extract_pex" $args
  set max_res 50
  set threshold 0.1
  set ratio 0.03
  if { [info exists keys(-max_res)] } {
    set max_res [pex::check_not_negative -max_res $keys(-max_res)]
  }
  if { [info exists keys(-coupling_threshold)] } {
    set threshold [pex::check_not_negative -coupling_threshold \
      $keys(-coupling_threshold)]
  }
  if { [info exists keys(-coupling_ratio)] } {
    set ratio [pex::check_not_negative -coupling_ratio \
      $keys(-coupling_ratio)]
  }
  pex::extract_cmd $max_res $threshold $ratio
}

sta::define_cmd_args "write_pex_spef" {[-nets nets] filename}

proc write_pex_spef { args } {
  sta::parse_key_args "write_pex_spef" args \
    keys {-nets} flags {}
  sta::check_argc_eq1 "write_pex_spef" $args
  set nets {}
  if { [info exists keys(-nets)] } {
    set nets $keys(-nets)
  }
  pex::write_spef_cmd [file nativename [lindex $args 0]] $nets
}

sta::define_cmd_args "compare_pex_fsv" {[-nets nets]\
                                         [-sample count]\
                                         [-halo halo]\
                                         [-tol tolerance]}

proc compare_pex_fsv { args } {
  sta::parse_key_args "compare_pex_fsv" args \
    keys {-nets -sample -halo -tol} flags {}
  sta::check_argc_eq0 "compare_pex_fsv" $args
  set nets {}
  set sample 0
  if { [info exists keys(-nets)] } {
    if { [info exists keys(-sample)] } {
      utl::error PEX 34 "-nets and -sample cannot both be given."
    }
    set nets $keys(-nets)
  } elseif { [info exists keys(-sample)] } {
    set sample $keys(-sample)
    if { !([string is integer -strict $sample] && $sample > 0) } {
      utl::error PEX 35 "-sample must be an integer greater than 0."
    }
  } else {
    utl::error PEX 36 "Give -nets or -sample."
  }
  set halo 10
  if { [info exists keys(-halo)] } {
    set halo [pex::check_not_negative -halo $keys(-halo)]
  }
  set tol 0.005
  if { [info exists keys(-tol)] } {
    set tol [pex::check_positive -tol $keys(-tol)]
  }
  pex::compare_cmd $nets $sample $halo $tol
}

namespace eval pex {
proc required_key { key keys_list } {
  array set keys $keys_list
  if { ![info exists keys($key)] } {
    utl::error PEX 30 "$key is required."
  }
  return $keys($key)
}

proc check_positive { key value } {
  if { !([string is double -strict $value] && $value > 0) } {
    utl::error PEX 31 "$key must be a number greater than 0."
  }
  return $value
}

proc check_not_negative { key value } {
  if { !([string is double -strict $value] && $value >= 0) } {
    utl::error PEX 32 "$key must be a number of at least 0."
  }
  return $value
}
}

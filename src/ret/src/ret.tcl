# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

sta::define_cmd_args "retime" {[-max_moves count]}

proc retime { args } {
  sta::parse_key_args "retime" args keys {-max_moves} flags {}
  sta::check_argc_eq0 "retime" $args

  set max_moves 0
  if { [info exists keys(-max_moves)] } {
    set max_moves $keys(-max_moves)
    sta::check_positive_integer "-max_moves" $max_moves
  }

  ret::retime_cmd $max_moves
}

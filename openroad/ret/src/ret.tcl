# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

sta::define_cmd_args "retime" {[-max_moves count]\
                               [-margin fraction]\
                               [-move_named_registers]}

proc retime { args } {
  sta::parse_key_args "retime" args \
    keys {-max_moves -margin} flags {-move_named_registers}
  sta::check_argc_eq0 "retime" $args

  set max_moves 0
  if { [info exists keys(-max_moves)] } {
    set max_moves $keys(-max_moves)
    sta::check_positive_integer "-max_moves" $max_moves
  }
  set margin 0.002
  if { [info exists keys(-margin)] } {
    set margin $keys(-margin)
    sta::check_positive_float "-margin" $margin
  }

  ret::retime_cmd $max_moves $margin [info exists flags(-move_named_registers)]
}

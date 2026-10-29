# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

sta::define_cmd_args "unmap_sequential_cells" {}

proc unmap_sequential_cells { args } {
  sta::parse_key_args "unmap_sequential_cells" args keys {} flags {}
  sta::check_argc_eq0 "unmap_sequential_cells" $args

  ret::unmap_sequential_cells_cmd
}

sta::define_cmd_args "retime" {[-max_moves count] [-objective min|period]}

proc retime { args } {
  sta::parse_key_args "retime" args keys {-max_moves -objective} flags {}
  sta::check_argc_eq0 "retime" $args

  set max_moves 0
  if { [info exists keys(-max_moves)] } {
    set max_moves $keys(-max_moves)
    sta::check_positive_integer "-max_moves" $max_moves
  }

  set stop_at_period 0
  if { [info exists keys(-objective)] } {
    switch $keys(-objective) {
      "min" { set stop_at_period 0 }
      "period" { set stop_at_period 1 }
      default { utl::error RET 31 "-objective must be min or period" }
    }
  }

  ret::retime_cmd $max_moves $stop_at_period
}

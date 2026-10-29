# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

sta::define_cmd_args "unmap_sequential_cells" {}

proc unmap_sequential_cells { args } {
  sta::parse_key_args "unmap_sequential_cells" args keys {} flags {}
  sta::check_argc_eq0 "unmap_sequential_cells" $args

  ret::unmap_sequential_cells_cmd
}

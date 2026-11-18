# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# Example user-level command with argument processing.

sta::define_cmd_args "example_instance" { \
                           [-name name]}

proc example_instance { args } {
  sta::parse_key_args "example_instance" args \
    keys {-name} flags {}

  set name "example"
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }

  sta::check_argc_eq0 "example_instance" $args

  exa::make_instance_cmd $name
}

namespace eval exa {
# Example private command (not in the global namespace) for debugging.
proc set_debug { args } {
  sta::parse_key_args "example_debug" args \
    keys {} flags {}

  exa::set_debug_cmd
}
}

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

# Multithreaded exercise of logToDb, logToDbBulk, and logToDbMetadata.
# Both tables share the same idx range [0, num_entries) so they can be
# cross-indexed via a JOIN on the idx column.
proc db_log_test { args } {
  sta::parse_key_args "db_log_test" args \
    keys {-threads -num_entries -chunks} flags {}

  set num_threads 4
  if { [info exists keys(-threads)] } {
    set num_threads $keys(-threads)
  }
  set num_entries 100000
  if { [info exists keys(-num_entries)] } {
    set num_entries $keys(-num_entries)
  }
  set num_chunks 4
  if { [info exists keys(-chunks)] } {
    set num_chunks $keys(-chunks)
  }

  sta::check_argc_eq0 "db_log_test" $args

  exa::db_log_test_cmd $num_threads $num_entries $num_chunks
}
}

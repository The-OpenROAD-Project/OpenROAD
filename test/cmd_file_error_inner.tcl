# Helper for cmd_file_error.tcl.  Fails inside a proc, two frames down,
# so that the error has a stack trace worth reporting.

proc cmd_file_error_inner { x } {
  return [expr { $x / 0 }]
}

proc cmd_file_error_outer { } {
  cmd_file_error_inner 3
}

cmd_file_error_outer

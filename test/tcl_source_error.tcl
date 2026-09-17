# Helper for tcl_source.tcl.  Fails inside a proc, two frames down, so
# that the error has a stack trace worth reporting.

proc tcl_source_inner { x } {
  return [expr { $x / 0 }]
}

proc tcl_source_outer { } {
  tcl_source_inner 3
}

tcl_source_outer

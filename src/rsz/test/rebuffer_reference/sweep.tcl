# Time evenly spaced chains of one buffer size for every length; the best
# of these seed search.tcl.
#
# Environment: REF_SIZE_DRIVER (0 or 1), REF_OUT (output file).
# Run from src/rsz/test.

source "rebuffer_reference/common.tcl"
ref::init

if { $::env(REF_SIZE_DRIVER) } {
  set bare_drivers $ref::cells
  set chain_drivers {BUF_X1 BUF_X4 BUF_X8 BUF_X16}
} else {
  set bare_drivers {BUF_X1}
  set chain_drivers {BUF_X1}
}

for { set i 1 } { $i <= 100 } { incr i } {
  foreach driver $bare_drivers {
    ref::make_path $i $driver {} {}
  }
  # Up to one buffer per 250 um plus two, at most nine.
  set max_bufs [expr { min(9, int($i * 25 / 250.0) + 2) }]
  for { set k 1 } { $k <= $max_bufs } { incr k } {
    foreach cell $ref::cells {
      foreach driver $chain_drivers {
        ref::make_path $i $driver [lrepeat $k $cell] [ref::uniform_xs $i $k]
      }
    }
  }
}
ord::design_created
ref::constrain
ref::time_all

set out [open $::env(REF_OUT) w]
for { set p 0 } { $p < $ref::npaths } { incr p } {
  puts $out [ref::format_solution $p]
}
close $out

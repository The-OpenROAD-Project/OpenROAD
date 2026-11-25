# Time evenly spaced chains of one buffer size for every length; the best
# of these seed search.tcl.
#
# Environment: REF_PDK, REF_SIZE_DRIVER (0 or 1), REF_OUT (output file).
# Run from openroad/rsz/test.

source "rebuffer_reference/common.tcl"
ref::init

if { $::env(REF_SIZE_DRIVER) } {
  set bare_drivers $ref::cells
  set chain_drivers $bvl_ref_drivers
} else {
  set bare_drivers [list $bvl_buffer]
  set chain_drivers [list $bvl_buffer]
}

for { set i 1 } { $i <= 100 } { incr i } {
  foreach driver $bare_drivers {
    ref::make_path $i $driver {} {}
  }
  # Up to one buffer per ten length steps plus two, at most nine.
  set max_bufs [expr { min(9, int($i / 10.0) + 2) }]
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

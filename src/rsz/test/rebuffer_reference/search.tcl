# Local search for the minimum-delay solution of every length.
#
# Seeds per length: rsz's solution, the best sweep.tcl chains, and chains
# that start with a buffer at the driver. Every path is then improved by
# coordinate descent: move one buffer by +/-step or resize it (or the driver,
# when sizing) by one drive strength, and keep the change if STA says the
# path got faster. All paths take their trial move in the same STA run.
#
# Environment: REF_SIZE_DRIVER (0 or 1), REF_RSZ (rsz_solution.tcl output),
# REF_SWEEP (sweep.tcl output), REF_OUT (best solution per length).
# Run from src/rsz/test.

source "rebuffer_reference/common.tcl"
ref::init
set size_driver $::env(REF_SIZE_DRIVER)

proc read_solutions { file_name } {
  set solutions {}
  set in [open $file_name]
  while { [gets $in line] >= 0 } {
    lappend solutions $line
  }
  close $in
  return $solutions
}

proc make_solution_path { solution } {
  lassign $solution i delay driver chain
  set seq {}
  set xs {}
  foreach buf $chain {
    lappend seq [lindex $buf 0]
    lappend xs [lindex $buf 1]
  }
  return [ref::make_path $i $driver $seq $xs]
}

foreach solution [read_solutions $::env(REF_RSZ)] {
  make_solution_path $solution
}
foreach solution [read_solutions $::env(REF_SWEEP)] {
  lappend sweep([lindex $solution 0]) $solution
}
for { set i 1 } { $i <= 100 } { incr i } {
  set top [lrange [lsort -real -index 1 $sweep($i)] 0 7]
  foreach solution $top {
    make_solution_path $solution
  }
  set best_count [llength [lindex $top 0 3]]
  foreach k [list [expr { $best_count - 1 }] $best_count [expr { $best_count + 1 }]] {
    if { $k < 0 } {
      continue
    }
    foreach first {BUF_X4 BUF_X8 BUF_X16} {
      if { $k == 0 } {
        ref::make_path $i BUF_X1 [list $first] {930}
        continue
      }
      foreach cell {BUF_X4 BUF_X8} {
        set seq [concat [list $first] [lrepeat $k $cell]]
        set xs [concat {930} [ref::uniform_xs $i $k]]
        ref::make_path $i BUF_X1 $seq $xs
      }
    }
  }
}
ord::design_created
ref::constrain
ref::time_all
puts "paths $ref::npaths"

# Apply move kind (0: +x, 1: -x, 2: upsize, 3: downsize) to buffer j of
# path p (j = -1 is the driver); returns an undo record or {} if invalid.
proc try_move { p j kind step } {
  set bufs $ref::path($p,bufs)
  if { $j >= [llength $bufs] } {
    return {}
  }
  if { $j < 0 } {
    if { $kind < 2 } {
      return {}
    }
    set inst $ref::path($p,drvr)
  } else {
    set inst [lindex $bufs $j]
  }
  if { $kind < 2 } {
    set x [lindex [$inst getLocation] 0]
    set new_x [expr { $kind == 0 ? $x + $step : $x - $step }]
    set lo 0
    if { $j > 0 } {
      set lo [lindex [[lindex $bufs [expr { $j - 1 }]] getLocation] 0]
    }
    set hi $ref::path($p,length)
    if { $j < [llength $bufs] - 1 } {
      set hi [lindex [[lindex $bufs [expr { $j + 1 }]] getLocation] 0]
    }
    if { $new_x < $lo || $new_x > $hi } {
      return {}
    }
    $inst setLocation $new_x $ref::path($p,y)
    return [list location $inst $x]
  }
  set cell [[$inst getMaster] getName]
  set index [lsearch $ref::cells $cell]
  set new_index [expr { $kind == 2 ? $index + 1 : $index - 1 }]
  if { $new_index < 0 || $new_index >= [llength $ref::cells] } {
    return {}
  }
  $inst swapMaster $ref::masters([lindex $ref::cells $new_index])
  return [list master $inst $cell]
}

proc undo_move { p undo } {
  lassign $undo what inst value
  if { $what == "location" } {
    $inst setLocation $value $ref::path($p,y)
  } else {
    $inst swapMaster $ref::masters($value)
  }
}

set max_bufs 0
for { set p 0 } { $p < $ref::npaths } { incr p } {
  set max_bufs [expr { max($max_bufs, [llength $ref::path($p,bufs)]) }]
  set best_delay($p) $ref::delay($p)
}
set first_j [expr { $size_driver ? -1 : 0 }]

# Steps in DBU (2000 per um): 51.2 um down to 0.2 um.
foreach step {102400 51200 25600 12800 6400 3200 1600 800 400} {
  for { set sweep_count 0 } { $sweep_count < 4 } { incr sweep_count } {
    set improved 0
    for { set j $first_j } { $j < $max_bufs } { incr j } {
      foreach kind {0 1 2 3} {
        set undos {}
        for { set p 0 } { $p < $ref::npaths } { incr p } {
          set undo [try_move $p $j $kind $step]
          if { $undo != {} } {
            lappend undos $p $undo
          }
        }
        if { $undos == {} } {
          continue
        }
        ref::time_all
        foreach {p undo} $undos {
          if { $ref::delay($p) < $best_delay($p) - 1e-4 } {
            set best_delay($p) $ref::delay($p)
            incr improved
          } else {
            undo_move $p $undo
          }
        }
      }
    }
    puts "step $step sweep $sweep_count improved $improved"
    if { $improved == 0 } {
      break
    }
  }
}
ref::time_all

for { set p 0 } { $p < $ref::npaths } { incr p } {
  set i $ref::path($p,i)
  if { ![info exists best($i)] || $ref::delay($p) < $ref::delay($best($i)) } {
    set best($i) $p
  }
}
set out [open $::env(REF_OUT) w]
for { set i 1 } { $i <= 100 } { incr i } {
  puts $out [ref::format_solution $best($i)]
}
close $out

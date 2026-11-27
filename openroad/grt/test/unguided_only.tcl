# global_route -unguided_only keeps every net's guides and routes only
# the nets that have none
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

global_route

proc guides_of { net } {
  set boxes {}
  foreach g [$net getGuides] {
    set b [$g getBox]
    lappend boxes "[[$g getLayer] getName] [$b xMin] [$b yMin] [$b xMax] [$b yMax]"
  }
  return [lsort $boxes]
}

proc changed_nets { before } {
  set changed 0
  foreach net [[ord::get_db_block] getNets] {
    if { [guides_of $net] ne [dict get $before [$net getName]] } {
      incr changed
    }
  }
  return $changed
}

set block [ord::get_db_block]
set cleared {}
foreach name {clk _000_ _001_ _002_ {resp_msg[0]}} {
  set net [$block findNet $name]
  $net clearGuides
  lappend cleared $net
}
set before {}
foreach net [$block getNets] {
  dict set before [$net getName] [guides_of $net]
}

global_route -unguided_only

foreach net $cleared {
  check "[$net getName] has guides again" {
    expr { [llength [$net getGuides]] > 0 }
  } 1
}
check "only the cleared nets have new guides" {
  changed_nets $before
} [llength $cleared]

set before {}
foreach net [$block getNets] {
  dict set before [$net getName] [guides_of $net]
}
global_route -unguided_only
check "with every net guided, nothing is rerouted" {
  changed_nets $before
} 0

foreach option {-start_incremental -end_incremental -congestion_only} {
  check "-unguided_only $option fails with GRT-0345" {
    set rc [catch { global_route -unguided_only $option } err]
    expr { $rc != 0 && [string first "GRT-0345" $err] >= 0 }
  } 1
}

exit_summary

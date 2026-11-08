# The global_placement flags and the uniform density command reach the
# placer. Placement quality is covered by nesterov_place_test.
source helpers.tcl
read_lef ./nangate45.lef
read_def ./gcd.def

set block [ord::get_db_block]
set input {}
foreach inst [$block getInsts] {
  lappend input $inst [$inst getLocation] [$inst getPlacementStatus]
}

proc restore_input { } {
  foreach { inst location status } $::input {
    $inst setLocation {*}$location
    $inst setPlacementStatus $status
  }
}

# Places from the input and returns the log.
proc place { args } {
  restore_input
  tee -quiet -variable log [list global_placement {*}$args]
  return $log
}

proc iterations { log } {
  regexp {Global placement finished at iteration (\d+)} $log -> count
  return $count
}

proc near { a b } {
  return [expr { abs($a - $b) < 0.001 }]
}

check "uniform density" {
  near [gpl::get_global_placement_uniform_density] 0.65
} 1
check "uniform density with padding" {
  near [gpl::get_global_placement_uniform_density -pad_left 1 -pad_right 1] 0.85
} 1

set log [place -skip_initial_place]
set default_iterations [iterations $log]
check "initial place skipped" {
  regexp {\[InitialPlace\]  Iter:} $log
} 0
check "default bin count" { regexp {Bin count \(X, Y\):\s+16\s*,\s*16} $log } 1

set log [place -skip_nesterov_place]
check "nesterov place skipped" {
  regexp {Global placement finished} $log
} 0
check "initial place runs" { regexp {\[InitialPlace\]  Iter:} $log } 1

set log [place -skip_initial_place -bin_grid_count 32]
check "bin count" { regexp {Bin count \(X, Y\):\s+32\s*,\s*32} $log } 1

set log [place -skip_initial_place -density uniform]
regexp {Placement target density:\s+([0-9.]+)} $log -> target
check "uniform target density" { expr { $target < 0.7 } } 1

set log [place -skip_initial_place -overflow 0.2]
check "looser overflow stops sooner" {
  expr { [iterations $log] < $default_iterations }
} 1

# A reference far below the design's HPWL slows the penalty ramp.
set log [place -skip_initial_place -reference_hpwl 100000]
check "low reference HPWL slows convergence" {
  expr { [iterations $log] > $default_iterations }
} 1

# With -skip_io the IO pins are not placer pins.
set log [place]
regexp {Number of pins:\s+(\d+)} $log -> pins
set log [place -skip_io]
regexp {Number of pins:\s+(\d+)} $log -> pins_skip_io
check "skip_io drops the IO pins" {
  expr { $pins - $pins_skip_io }
} [llength [$block getBTerms]]
check "skip_io skips initial place" {
  regexp {\[InitialPlace\]  Iter:} $log
} 0

exit_summary

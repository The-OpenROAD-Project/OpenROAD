# Incremental timing awareness must run without resizing the optimized netlist.
source helpers.tcl
set test_name incremental_timing
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib
read_lef ./nangate45.lef
read_def ./simple01-td.def
create_clock -name core_clock -period 0.2 clk
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
set_thread_count 1

# Coincident locked cells keep phase 1 above the target, forcing the unlocked
# timing-aware pass. One genuinely unplaced cell makes this an incremental edit.
set block [ord::get_db_block]
set instances [$block getInsts]
foreach inst $instances {
  $inst setLocation 20000 20000
  $inst setPlacementStatus PLACED
}
set new_inst [lindex $instances 0]
$new_inst setPlacementStatus NONE
set original {}
foreach inst $instances {
  set pins {}
  foreach pin [$inst getITerms] {
    set net [$pin getNet]
    if { $net != "NULL" } {
      lappend pins [list [$pin getName] [$net getName]]
    }
  }
  lappend original [list [$inst getName] [[$inst getMaster] getName] $pins]
}

with_output_to_variable log_text {
  global_placement -incremental -density 0.7
}
puts $log_text
if { ![string match {*\[INFO GPL-0196\]*} $log_text] } {
  error "The timing-aware unlocked pass did not execute"
}
if {
  ![regexp {timing protection enabled: ([0-9]+) weighted nets, maximum weight ([0-9.]+)} \
    $log_text unused weighted_nets max_weight]
} {
  error "Missing actual timing weights from the unlocked pass"
}
if { $weighted_nets == 0 || $max_weight <= 1.0 } {
  error "The unlocked pass did not use non-unit critical-net weights"
}
set final {}
set moved 0
foreach inst [$block getInsts] {
  if { [$inst getPlacementStatus] == "NONE" } {
    error "Timing rollback left an instance unplaced"
  }
  if { [$inst getLocation] != {20000 20000} } {
    incr moved
  }
  set pins {}
  foreach pin [$inst getITerms] {
    set net [$pin getNet]
    if { $net != "NULL" } {
      lappend pins [list [$pin getName] [$net getName]]
    }
  }
  lappend final [list [$inst getName] [[$inst getMaster] getName] $pins]
}
if { $original != $final } {
  error "Incremental timing reweighting changed cell sizes or connectivity"
}
if { $moved == 0 } {
  error "Incremental placement did not actually move any cells"
}
puts pass

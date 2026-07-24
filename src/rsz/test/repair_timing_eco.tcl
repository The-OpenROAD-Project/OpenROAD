# repair_timing_eco -- ECO-driven timing-fix closure loop with change report.
#
# Asserts:
#   (a) WNS improves or stays equal (the fix never makes timing worse).
#   (b) the reported change counts match an independently computed netlist
#       delta (the audit trail is accurate, not estimated).
#   (c) a set_dont_touch instance is left untouched (not removed, master
#       unchanged, not reported as resized) -- clean parts of the design are
#       not perturbed even when the unconstrained flow would have removed it.
#
# Uses the same small Nangate45 reg1 design as eco_round_trip / repair_setup1.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set block [[[ord::get_db] getChip] getBlock]

# Independent netlist signature (mirrors the command's internal snapshot but is
# computed here in the test so we can cross-check the reported counts).
proc signature { } {
  set block [[[ord::get_db] getChip] getBlock]
  set insts [dict create]
  foreach inst [$block getInsts] {
    dict set insts [$inst getName] [[$inst getMaster] getName]
  }
  set nets [dict create]
  foreach net [$block getNets] {
    set conns {}
    foreach iterm [$net getITerms] {
      lappend conns "[[$iterm getInst] getName]/[[$iterm getMTerm] getName]"
    }
    dict set nets [$net getName] [lsort $conns]
  }
  return [list $insts $nets]
}

# u2 is one of the buffers the unconstrained flow removes (u2..u5 in
# repair_setup1.ok).  Marking it dont_touch must keep it in the netlist, while
# the rest of the fix (e.g. resizing r1) still proceeds.
set u2 [$block findInst u2]
set u2_master_before [[$u2 getMaster] getName]
set_dont_touch u2

set wns_before [rsz::eco_wns]
set snap_before [signature]

# Run the closure command (also persists the ECO to a file).
set eco_file [make_result_file repair_timing_eco.eco]
set delta [repair_timing_eco -eco_file $eco_file -setup]

set wns_after [rsz::eco_wns]
set snap_after [signature]

# (a) WNS not worse.
puts "wns improved or equal [expr { $wns_after >= $wns_before ? "yes" : "no" }]"

# (b) reported counts match an independently computed delta.
set indep [rsz::eco_compute_delta $snap_before $snap_after]
set match 1
foreach key {bufs_inserted bufs_removed insts_added insts_removed nets_modified} {
  if { [dict get $delta $key] != [dict get $indep $key] } {
    set match 0
  }
}
if { [llength [dict get $delta resized]] != [llength [dict get $indep resized]] } {
  set match 0
}
puts "reported counts match netlist delta [expr { $match ? "yes" : "no" }]"
puts "resized [llength [dict get $delta resized]]"
puts "bufs_removed [dict get $delta bufs_removed]"
puts "bufs_inserted [dict get $delta bufs_inserted]"
puts "nets_modified [dict get $delta nets_modified]"

# Show the before/after of the resized gate(s) (audit trail content).
foreach r [dict get $delta resized] {
  puts "resized detail [lindex $r 0] [lindex $r 1] -> [lindex $r 2]"
}

# (c)/(d) dont_touch instance u2 untouched: still present, master unchanged,
# and not removed by the loop.
set u2_after [$block findInst u2]
puts "u2 still present [expr { $u2_after ne "NULL" ? "yes" : "no" }]"
if { $u2_after ne "NULL" } {
  set u2_master_after [[$u2_after getMaster] getName]
  puts "u2 master unchanged [expr { $u2_master_after eq $u2_master_before ? "yes" : "no" }]"
}
# u2 must not appear in the resized list either.
set u2_resized 0
foreach r [dict get $delta resized] {
  if { [lindex $r 0] eq "u2" } {
    set u2_resized 1
  }
}
puts "u2 not resized [expr { $u2_resized ? "no" : "yes" }]"

# ECO file persisted.
set eco_ok [expr { [file exists $eco_file] && [file size $eco_file] > 0 }]
puts "eco file non-empty [expr { $eco_ok ? "yes" : "no" }]"

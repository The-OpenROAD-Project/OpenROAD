# repair_hold_eco -- dedicated HOLD-violation repair pass (additive/opt-in).
#
# Uses the sky130hs design from repair_hold4, which has a known hold violation
# (worst hold slack -0.14 ns) while setup is comfortably positive (1.79 ns).
#
# Asserts:
#   (a) the hold violation is fixed: hold WNS goes from < 0 to >= 0.
#   (b) at least one delay buffer is inserted (the hold-repair mechanism), and
#       the reported count matches an independently computed netlist delta.
#   (c) setup is NOT degraded -- setup WNS after >= setup WNS before (the
#       classic hold-fix hazard), which is also what the command asserts.
#   (d) a set_dont_touch instance is left untouched (still present, master
#       unchanged, not reported as resized).
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def repair_hold4.def

create_clock -period 2 clk
set_propagated_clock clk

source sky130hs/sky130hs.rc
set_wire_rc -layer met2
estimate_parasitics -placement

set block [[[ord::get_db] getChip] getBlock]

# Independent netlist signature so we can cross-check the reported counts.
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

# i4 is a clock-tree buffer that is not on the repaired data path; marking it
# dont_touch must leave it untouched while the hold fix still proceeds.
set i4 [$block findInst i4]
set i4_master_before [[$i4 getMaster] getName]
set_dont_touch i4

set hold_before [rsz::eco_hold_wns]
set setup_before [rsz::eco_wns]
set snap_before [signature]

# Run the dedicated hold-repair closure command (also persists the ECO).
set eco_file [make_result_file repair_hold_eco.eco]
set delta [repair_hold_eco -eco_file $eco_file]

set hold_after [rsz::eco_hold_wns]
set setup_after [rsz::eco_wns]
set snap_after [signature]

# (a) hold violation fixed: was negative, now non-negative.
puts "hold violated before [expr { $hold_before < 0.0 ? "yes" : "no" }]"
puts "hold fixed after [expr { $hold_after >= 0.0 ? "yes" : "no" }]"

# (b) at least one buffer inserted, and reported counts match independent delta.
set indep [rsz::eco_compute_delta $snap_before $snap_after]
set match 1
foreach key {bufs_inserted bufs_removed insts_added insts_removed nets_modified} {
  if { [dict get $delta $key] != [dict get $indep $key] } {
    set match 0
  }
}
puts "buffers inserted [expr { [dict get $delta bufs_inserted] >= 1 ? "yes" : "no" }]"
puts "reported counts match netlist delta [expr { $match ? "yes" : "no" }]"

# (c) setup not degraded.
puts "setup not degraded [expr { $setup_after >= $setup_before ? "yes" : "no" }]"

# (d) dont_touch instance i4 untouched.
set i4_after [$block findInst i4]
puts "i4 still present [expr { $i4_after ne "NULL" ? "yes" : "no" }]"
if { $i4_after ne "NULL" } {
  set i4_master_after [[$i4_after getMaster] getName]
  puts "i4 master unchanged [expr { $i4_master_after eq $i4_master_before ? "yes" : "no" }]"
}
set i4_resized 0
foreach r [dict get $delta resized] {
  if { [lindex $r 0] eq "i4" } {
    set i4_resized 1
  }
}
puts "i4 not resized [expr { $i4_resized ? "no" : "yes" }]"

set eco_size [file size $eco_file]
puts "eco file non-empty [expr { $eco_size > 0 ? "yes" : "no" }]"

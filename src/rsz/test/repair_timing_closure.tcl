# repair_timing_closure -- unified setup+hold ECO convergence loop.
#
# Composition test: the loop iterates the existing setup engine
# (repair_timing -setup), ECO legalization (improve_eco_legalization), the
# existing hold engine (repair_timing -hold) and incremental STA to drive BOTH
# setup and hold to closure on one design that starts with BOTH violated.
#
# Design: gcd_nangate45_placed at a tight period (0.58 ns) with a hold-side
# clock uncertainty of 0.2 ns.  This produces a real placed design that starts
# with BOTH a setup violation (WNS < 0 on the max path) AND a hold violation
# (WNS < 0 on the min path) -- the only configuration that actually exercises
# the unified loop (vs. the single-sided repair_timing_eco / repair_hold_eco).
#
# Asserts:
#   (a) BOTH violated before: setup WNS < 0 and hold WNS < 0.
#   (b) CONVERGENCE: both setup WNS >= 0 and hold WNS >= 0 after the loop.
#   (c) TERMINATION: the loop stops within max_iters (stop reason is a clean
#       terminal state, not the cap being hit blindly).
#   (d) NO SETUP REGRESSION: setup WNS after >= setup WNS before, and EVERY
#       per-iteration trajectory row keeps setup WNS >= the baseline -- the
#       explicit anti-ping-pong invariant (a hold fix never re-breaks setup
#       below where the loop started).
#   (e) ANTI-PING-PONG / MONOTONIC-ISH: the trajectory's setup WNS is
#       non-decreasing across iterations (no oscillation), proving the loop
#       does not bounce setup up and down chasing hold.
#   (f) dont_touch is respected: a marked instance is still present with its
#       master unchanged after the loop.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
create_clock [get_ports clk] -name core_clock -period 0.58
# Hold-side uncertainty manufactures a hold violation alongside the setup
# violation the tight period already produces.
set_clock_uncertainty -hold 0.2 [get_clocks core_clock]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# The ECO legalizer prints a wall-clock runtime line (DPL-1116) that is
# inherently nondeterministic (0.00s vs 0.01s); suppress it so the golden log
# is stable.  This does not affect the legalization itself.
suppress_message DPL 1116

set block [[[ord::get_db] getChip] getBlock]

# Mark an instance dont_touch and remember its master so we can prove the loop
# never perturbs it.
set dt_name "_440_"
set dt_inst [$block findInst $dt_name]
set dt_master_before [[$dt_inst getMaster] getName]
set_dont_touch $dt_name

set setup_before [rsz::eco_wns]
set hold_before [rsz::eco_hold_wns]

# (a) both violated at the start.
puts "setup violated before [expr { $setup_before < 0.0 ? "yes" : "no" }]"
puts "hold violated before [expr { $hold_before < 0.0 ? "yes" : "no" }]"

# Run the unified closure loop.
set max_iters 10
set d [repair_timing_closure -max_iters $max_iters -verbose]

set setup_after [dict get $d setup_wns_after]
set hold_after [dict get $d hold_wns_after]
set iters_run [dict get $d iters_run]
set stop_reason [dict get $d stop_reason]
set traj [dict get $d trajectory]

set eps 1e-12

# (b) convergence: both clean.
puts "setup closed after [expr { $setup_after >= -$eps ? "yes" : "no" }]"
puts "hold closed after [expr { $hold_after >= -$eps ? "yes" : "no" }]"

# (c) termination within the cap.
puts "terminated within max_iters [expr { $iters_run <= $max_iters ? "yes" : "no" }]"
puts "stop reason converged [expr { $stop_reason eq "converged" ? "yes" : "no" }]"

# (d) no setup regression vs baseline, overall and per-iteration.
puts "setup not regressed overall [expr { $setup_after >= $setup_before - $eps ? "yes" : "no" }]"
set all_rows_ok 1
foreach row $traj {
  set row_setup [lindex $row 1]
  if { $row_setup < $setup_before - $eps } {
    set all_rows_ok 0
  }
}
puts "every iteration keeps setup >= baseline [expr { $all_rows_ok ? "yes" : "no" }]"

# (e) anti-ping-pong: setup WNS is non-decreasing across the trajectory.
set monotonic 1
set prev_setup ""
foreach row $traj {
  set row_setup [lindex $row 1]
  if { $prev_setup ne "" && $row_setup < $prev_setup - $eps } {
    set monotonic 0
  }
  set prev_setup $row_setup
}
puts "setup trajectory non-decreasing [expr { $monotonic ? "yes" : "no" }]"

# (f) dont_touch respected.
set dt_after [$block findInst $dt_name]
puts "dont_touch present [expr { $dt_after ne "NULL" ? "yes" : "no" }]"
if { $dt_after ne "NULL" } {
  set dt_master_after [[$dt_after getMaster] getName]
  puts "dont_touch master unchanged [expr { $dt_master_after eq $dt_master_before ? "yes" : "no" }]"
}

# Report the trajectory length so the no-infinite-loop property is visible.
puts "trajectory rows [llength $traj]"
puts "iterations run $iters_run"

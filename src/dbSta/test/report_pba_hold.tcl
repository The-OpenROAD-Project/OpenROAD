# report_pba_slack -hold + closure decision: PBA hold-path pessimism recovery.
#
# Design: pba_gcd with a hold clock uncertainty of 0.1116 ns chosen so that
# GBA reports two hold-failing endpoints, of which PBA recovers exactly one
# (a GBA-pessimism artifact) while the other remains a genuine violation.
# This exercises the full closure decision surface (negative GBA slack that
# does / does not flip positive after PBA pessimism recovery).
#
# Verifies:
#  (a) the hold report runs and produces the expected column headers,
#  (b) PBA slack >= GBA slack on every reported HOLD path (recovered >= 0),
#  (c) the engineered endpoints behave as designed: one GBA-failing endpoint
#      recovers to positive (artifact), one stays negative (genuine), so the
#      closure summary is "10 endpoints, 2 GBA-failing, 1 recovered, 1
#      genuine violation",
#  (d) report_checks (GBA) min-path output is UNCHANGED by running the PBA
#      commands (additive-only guarantee).
#
# Float values are intentionally NOT echoed to stdout (they are platform/
# delay-calc dependent); the test asserts the invariants in Tcl and prints
# only deterministic PASS/FAIL booleans + counts + the column headers.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def pba_gcd.def

create_clock [get_ports clk] -name core_clock -period 0.485
# Hold uncertainty positioned between the GBA and PBA hold data-arrival of
# the two worst hold endpoints so the closure decision has both a genuine
# violation and a recovered (pessimism-artifact) endpoint.
set_clock_uncertainty -hold 0.1116 [get_clocks core_clock]

# --- (d) Capture GBA report_checks (min) output BEFORE running any PBA cmd.
with_output_to_variable gba_before {
  report_checks -path_delay min -fields {slew cap} -digits 4
}

# --- (a) Human-readable hold endpoint report -> echo only the header lines.
with_output_to_variable report {
  report_pba_slack -hold -endpoints -max_paths 10
}
foreach rline [split $report "\n"] {
  if { [string match "*endpoint recovery -- hold (min)" $rline] } {
    puts "endpoint header: [string trim $rline]"
  }
  if { [string match "Endpoint*GBA slack*PBA slack*Recovered*Status" $rline] } {
    puts "column header: $rline"
  }
}

# --- (b) Machine-readable verification of the PBA >= GBA invariant (hold).
set lines [sta::pba_endpoint_report_lines 10 "min"]
set n_eps [llength $lines]

set all_ok 1
set min_recovered 1e30
foreach line $lines {
  lassign $line endpoint gba pba recovered gbav pbav
  if { $recovered < -1e-15 } {
    set all_ok 0
    puts "VIOLATION: $endpoint recovered=$recovered < 0"
  }
  if { [expr {$pba - $gba}] < -1e-15 } {
    set all_ok 0
    puts "VIOLATION: $endpoint pba=$pba < gba=$gba"
  }
  if { $recovered < $min_recovered } {
    set min_recovered $recovered
  }
}

puts "endpoints found == 10: [expr {$n_eps == 10}]"
puts "invariant pba>=gba on all hold paths: $all_ok"
puts "min recovered non-negative: [expr {$min_recovered >= -1e-15}]"

# --- (c) Closure decision: deterministic counts from the engineered design.
# summary = "<endpoints> <gba_violations> <recovered> <pba_violations>"
set summary [sta::pba_closure_summary 10 "min"]
puts "closure summary: $summary"
lassign $summary s_eps s_gba_viol s_recovered s_pba_viol
puts "closure: 2 GBA-failing endpoints: [expr {$s_gba_viol == 2}]"
puts "closure: exactly 1 endpoint recovered by PBA: [expr {$s_recovered == 1}]"
puts "closure: exactly 1 genuine violation remains: [expr {$s_pba_viol == 1}]"
# recovered + remaining must account for every GBA failure (no endpoint lost).
puts "closure: recovered + genuine == gba-failing: \
[expr {$s_recovered + $s_pba_viol == $s_gba_viol}]"

# The genuine-violation list must contain only PBA-failing endpoints.
set closure_viols 0
foreach line $lines {
  lassign $line endpoint gba pba recovered gbav pbav
  if { $pbav == 1 } { incr closure_viols }
}
puts "closure: genuine-violation count matches pba_viol flags: \
[expr {$closure_viols == $s_pba_viol}]"

# --- (d) GBA report_checks (min) output must be identical after PBA cmds.
# Capture the closure report (it contains platform-dependent float slacks)
# so only deterministic booleans/counts reach the .ok file.
with_output_to_variable closure_report {
  report_pba_closure -hold -max_paths 10
}
# Echo only the deterministic closure trailer line shape (no float values).
foreach rline [split $closure_report "\n"] {
  if { [string match "PBA closure decision -- hold (min)" $rline] } {
    puts "closure header: [string trim $rline]"
  }
}
with_output_to_variable gba_after {
  report_checks -path_delay min -fields {slew cap} -digits 4
}
puts "gba report_checks (min) unchanged: [expr {$gba_before eq $gba_after}]"

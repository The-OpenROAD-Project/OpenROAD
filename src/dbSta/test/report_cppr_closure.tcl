# report_cppr_closure: CPPR slice 2 -- endpoint closure decision surface.
#
# Design (cppr.def, mirrors search_crpr.v): a reconvergent clock tree
#   clk -> ckbuf1 -> clk_buf1 -> {reg1/CK, ckbuf2 -> clk_buf2 -> reg2/CK}
# so the reg1 -> reg2 check shares the clk -> ckbuf1 clock segment. Under OCV
# derate that common segment is double-counted and CPPR credits the pessimism
# back. The clock period (0.123 ns) is chosen so GBA reports THREE setup
# endpoints failing under RAW (no-CRPR) slack, of which exactly ONE
# (reg2/D, the reconvergent reg1 -> reg2 check) is recovered to positive once
# the CPPR common-path credit is applied -- a clock-reconvergence-pessimism
# ARTIFACT -- while the other two (reg1/D, out1) have NO shared clock path,
# get zero credit, and stay GENUINE post-CPPR violations.
#
# This exercises the full slice-2 closure surface:
#   (1) CPPR-adjusted endpoint slack aggregation (raw / credit / adjusted),
#   (2) the closure decision separating CRPR artifacts from genuine
#       post-CPPR violations,
#   (3) report/analysis-only: report_checks (GBA) output is UNCHANGED.
#
# Verifies:
#  (a) the closure report runs and produces the expected column headers,
#  (b) credit >= 0 and cppr == raw + credit on every reported endpoint,
#  (c) the engineered endpoints behave as designed: reg2/D moves from the
#      genuine-violation list to the artifact (recovered) list once CPPR is
#      applied (raw < 0, CPPR-adjusted >= 0), and the two truly-failing
#      endpoints stay genuine, so the closure summary is
#      "3 endpoints, 3 raw-failing, 1 recovered, 2 genuine violations",
#  (d) the recovered endpoint is exactly reg2/D and the genuine list contains
#      only CPPR-failing endpoints,
#  (e) report_checks (GBA) max-path output is UNCHANGED by running the CPPR
#      closure commands (report/analysis-only guarantee).
#
# Float values are platform/delay-calc dependent, so they are NOT echoed to
# stdout; the test asserts the invariants in Tcl and prints only
# deterministic PASS/FAIL booleans + counts + the column headers.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def cppr.def

# Period tuned so the reconvergent reg1->reg2 endpoint (reg2/D) is the only
# one whose RAW failure is cleared by the CPPR common-path credit.
create_clock -name clk -period 0.123 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 1.0 [get_ports in1]
set_input_delay -clock clk 1.0 [get_ports in2]
set_output_delay -clock clk 2.0 [get_ports out1]

# OCV analysis + derate so CRPR is active in the GBA engine.
set_operating_conditions -analysis_type on_chip_variation
set_timing_derate -late 1.05
set_timing_derate -early 0.95

# --- (e) Capture GBA report_checks output BEFORE running any CPPR command.
with_output_to_variable gba_before {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}

# --- (a) Human-readable endpoint report -> echo only the header lines.
with_output_to_variable ep_report {
  report_cppr_closure -endpoints -max_paths 10
}
foreach rline [split $ep_report "\n"] {
  if { [string match "CPPR endpoint closure -- setup (max)" $rline] } {
    puts "endpoint header: [string trim $rline]"
  }
  if { [string match "Endpoint*Raw slack*CPPR slack*Credit*Status" $rline] } {
    puts "column header: $rline"
  }
}

# --- (b) Machine-readable verification of the CPPR invariants.
# line = endpoint raw cppr credit raw_viol cppr_viol
set lines [sta::cppr_endpoint_report_lines 10 "max"]
set n_eps [llength $lines]

set all_credit_nonneg 1
set all_sum_ok 1
foreach line $lines {
  lassign $line endpoint raw cppr credit rawv cpprv
  if { $credit < -1e-15 } {
    set all_credit_nonneg 0
    puts "VIOLATION: $endpoint credit=$credit < 0"
  }
  if { abs(($raw + $credit) - $cppr) > 1e-15 } {
    set all_sum_ok 0
    puts "VIOLATION: $endpoint raw+credit != cppr"
  }
}
puts "endpoints found == 3: [expr {$n_eps == 3}]"
puts "all credits non-negative: $all_credit_nonneg"
puts "cppr == raw + credit on all checks: $all_sum_ok"

# --- (c) Closure decision: deterministic counts from the engineered design.
# summary = "<endpoints> <raw_violations> <recovered> <cppr_violations>"
set summary [sta::cppr_closure_summary 10 "max"]
puts "closure summary: $summary"
lassign $summary s_eps s_raw_viol s_recovered s_cppr_viol
puts "closure: 3 raw-failing endpoints: [expr {$s_raw_viol == 3}]"
puts "closure: exactly 1 endpoint recovered by CPPR: [expr {$s_recovered == 1}]"
puts "closure: exactly 2 genuine violations remain: [expr {$s_cppr_viol == 2}]"
# recovered + remaining must account for every raw failure (no endpoint lost).
puts "closure: recovered + genuine == raw-failing: \
[expr {$s_recovered + $s_cppr_viol == $s_raw_viol}]"

# --- (d) The recovered endpoint must be reg2/D (the reconvergent check); the
# genuine list must contain only CPPR-failing endpoints, and reg2/D must have
# moved OUT of the genuine list (raw-failing but CPPR-passing).
set reg2_recovered 0
set reg2_raw_failing 0
set genuine_count 0
foreach line $lines {
  lassign $line endpoint raw cppr credit rawv cpprv
  if { $cpprv == 1 } { incr genuine_count }
  if { $endpoint eq "reg2/D" } {
    set reg2_raw_failing [expr { $rawv == 1 }]
    # recovered = raw-failing under GBA but passing after CPPR credit applied.
    set reg2_recovered [expr { $rawv == 1 && $cpprv == 0 }]
  }
}
puts "reg2/D raw-failing under GBA: $reg2_raw_failing"
puts "reg2/D moved to artifact (recovered) list after CPPR: $reg2_recovered"
puts "genuine-violation count matches cppr_viol summary: \
[expr {$genuine_count == $s_cppr_viol}]"

# --- (e) GBA report_checks output must be identical after the CPPR commands.
# Capture the closure report (it carries platform-dependent float slacks) so
# only deterministic booleans/counts reach the .ok file.
with_output_to_variable closure_report {
  report_cppr_closure -max_paths 10
}
foreach rline [split $closure_report "\n"] {
  if { [string match "CPPR closure decision -- setup (max)" $rline] } {
    puts "closure header: [string trim $rline]"
  }
}
with_output_to_variable gba_after {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
puts "gba report_checks unchanged: [expr {$gba_before eq $gba_after}]"

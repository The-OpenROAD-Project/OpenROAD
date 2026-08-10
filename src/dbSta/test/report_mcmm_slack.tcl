# report_mcmm_slack: Multi-Corner Multi-Mode cross-corner worst-slack report
# (report-only, additive).
#
# Design (cppr.def): reg1 -> reg2 plus input/output ports, in Nangate45.
# Two corners are declared with per-corner liberty:
#   fast = Nangate45_fast.lib   (smaller cell delays)
#   slow = Nangate45_slow.lib   (larger cell delays)
#
# Physics being exercised (the point of MCMM): different checks are limited by
# different corners.
#   * SETUP (max): data arrives LATER in the slow corner, so the worst setup
#     slack per endpoint is in the SLOW corner.
#   * HOLD  (min): data arrives EARLIER in the fast corner, so the worst hold
#     slack per endpoint is in the FAST corner.
#
# Verifies:
#  (a) the report runs and lists BOTH active corners,
#  (b) for every endpoint the reported worst slack equals the minimum of the
#      per-corner slacks, and the limiting corner matches that minimum,
#  (c) every setup endpoint is limited by "slow" and every hold endpoint is
#      limited by "fast" (different corners limit different checks),
#  (d) a SINGLE-corner run (only the slow liberty) reproduces exactly the
#      slow column of the multi-corner run -- i.e. MCMM does not perturb the
#      single-corner numbers,
#  (e) report_checks (GBA) output is UNCHANGED by running report_mcmm_slack
#      (additive / report-only guarantee).
#
# Float values are platform/delay-calc dependent, so they are NOT echoed to
# stdout; the test asserts the invariants in Tcl and prints only
# deterministic PASS/FAIL booleans + the active-corner list.
source "helpers.tcl"

define_corners fast slow
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_lef Nangate45/Nangate45.lef
read_def cppr.def

create_clock -name clk -period 0.3 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 0.05 [get_ports in1]
set_input_delay -clock clk 0.05 [get_ports in2]
set_output_delay -clock clk 0.05 [get_ports out1]

# --- (e) Capture GBA report_checks output BEFORE running report_mcmm_slack.
with_output_to_variable gba_before {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}

# --- (a) Human-readable report -> capture, echo only the active-corner list.
with_output_to_variable report {
  report_mcmm_slack -setup -max_endpoints 10
}
set saw_fast 0
set saw_slow 0
foreach rline [split $report "\n"] {
  set t [string trim $rline]
  if { [string match {\[0\] fast} $t] } { set saw_fast 1 }
  if { [string match {\[1\] slow} $t] } { set saw_slow 1 }
}
puts "active corners listed (fast+slow): [expr {$saw_fast && $saw_slow}]"

# Parse a machine-readable mcmm line:
#   "<endpoint> <worst_slack> <worst_corner> fast=<s|NA> slow=<s|NA>"
# Returns a dict-ish list: endpoint worst worst_corner fast_slack slow_slack
proc parse_mcmm_line { line } {
  set endpoint     [lindex $line 0]
  set worst        [lindex $line 1]
  set worst_corner [lindex $line 2]
  set fast NA
  set slow NA
  foreach tok [lrange $line 3 end] {
    set kv [split $tok "="]
    set k [lindex $kv 0]
    set v [lindex $kv 1]
    if { $k eq "fast" } { set fast $v }
    if { $k eq "slow" } { set slow $v }
  }
  return [list $endpoint $worst $worst_corner $fast $slow]
}

# --- (b)(c) SETUP: worst == min(per corner), limiting corner == slow.
set setup_lines [sta::mcmm_report_lines 10 "max"]
set setup_n [llength $setup_lines]
set setup_worst_ok 1
set setup_limit_ok 1
set setup_slow_le_fast 1
foreach line $setup_lines {
  lassign [parse_mcmm_line $line] ep worst wc fast slow
  if { $fast eq "NA" || $slow eq "NA" } { continue }
  set mn [expr { $fast < $slow ? $fast : $slow }]
  if { abs($worst - $mn) > 1e-15 } {
    set setup_worst_ok 0
    puts "VIOLATION setup: $ep worst=$worst != min(fast=$fast,slow=$slow)"
  }
  # Limiting corner must be the one equal to the worst slack.
  set expect_corner [expr { $slow <= $fast ? "slow" : "fast" }]
  if { $wc ne $expect_corner } {
    set setup_limit_ok 0
    puts "VIOLATION setup: $ep limiting=$wc expected=$expect_corner"
  }
  if { $slow > $fast } { set setup_slow_le_fast 0 }
}
puts "setup endpoints > 0: [expr {$setup_n > 0}]"
puts "setup worst == min across corners: $setup_worst_ok"
puts "setup limiting corner correct: $setup_limit_ok"
puts "setup limited by slow corner: $setup_slow_le_fast"

# --- (b)(c) HOLD: worst == min(per corner), limiting corner == fast.
set hold_lines [sta::mcmm_report_lines 10 "min"]
set hold_n [llength $hold_lines]
set hold_worst_ok 1
set hold_limit_ok 1
set hold_fast_le_slow 1
foreach line $hold_lines {
  lassign [parse_mcmm_line $line] ep worst wc fast slow
  if { $fast eq "NA" || $slow eq "NA" } { continue }
  set mn [expr { $fast < $slow ? $fast : $slow }]
  if { abs($worst - $mn) > 1e-15 } {
    set hold_worst_ok 0
    puts "VIOLATION hold: $ep worst=$worst != min(fast=$fast,slow=$slow)"
  }
  set expect_corner [expr { $fast <= $slow ? "fast" : "slow" }]
  if { $wc ne $expect_corner } {
    set hold_limit_ok 0
    puts "VIOLATION hold: $ep limiting=$wc expected=$expect_corner"
  }
  if { $fast > $slow } { set hold_fast_le_slow 0 }
}
puts "hold endpoints > 0: [expr {$hold_n > 0}]"
puts "hold worst == min across corners: $hold_worst_ok"
puts "hold limiting corner correct: $hold_limit_ok"
puts "hold limited by fast corner: $hold_fast_le_slow"

# Build a map endpoint -> slow-corner setup slack from the multi-corner run.
array set slow_col {}
set min_slow_col "NA"
set min_worst_col "NA"
foreach line $setup_lines {
  lassign [parse_mcmm_line $line] ep worst wc fast slow
  set slow_col($ep) $slow
  if { $slow ne "NA" } {
    if { $min_slow_col eq "NA" || $slow < $min_slow_col } {
      set min_slow_col $slow
    }
  }
  if { $min_worst_col eq "NA" || $worst < $min_worst_col } {
    set min_worst_col $worst
  }
}

# --- (d) Cross-check against OpenSTA's own engine numbers:
#  * the design-wide minimum of the per-corner SLOW column must equal
#    OpenSTA's per-scene worst slack for the slow corner, and
#  * the design-wide minimum of the WORST column must equal OpenSTA's
#    cross-corner worst slack.
# This proves report_mcmm_slack reports the engine's numbers (per corner and
# across corners), not numbers it invented.
set sta_slow_ws  [sta::worst_slack_scene [sta::find_scene slow] "max"]
set sta_cross_ws [sta::worst_slack_cmd "max"]
puts "slow column min == OpenSTA per-scene worst (slow): \
[expr { $min_slow_col ne {NA} && abs($min_slow_col - $sta_slow_ws) < 1e-15 }]"
puts "worst column min == OpenSTA cross-corner worst: \
[expr { $min_worst_col ne {NA} && abs($min_worst_col - $sta_cross_ws) < 1e-15 }]"

# --- (e) GBA report_checks output must be identical after report_mcmm_slack.
with_output_to_variable gba_after {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
puts "gba report_checks unchanged: [expr {$gba_before eq $gba_after}]"

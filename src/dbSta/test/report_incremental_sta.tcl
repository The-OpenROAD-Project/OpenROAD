# report_incremental_sta: incremental STA == full STA after an ECO edit
# (additive, report-only).  Design 1: cppr.def.
#
# Background
# ----------
# OpenSTA ALREADY recomputes timing incrementally after a netlist edit. When a
# cell is resized / swapped (replace_cell) the dbSta ODB callbacks call the Sta
# edit hooks (replaceEquivCellBefore/After, ...), which mark ONLY the affected
# vertices invalid. The next timing query runs a levelized BFS that touches
# only the changed fanout cone. report_incremental_sta is an ADDITIVE,
# report-only command: it identifies that affected cone by graph traversal,
# reports how many endpoints are in it vs. the full count, and reports the
# post-edit slacks taken from OpenSTA's own incremental query API.
#
# This test is the CORRECTNESS ORACLE on cppr.def. It:
#  (a) applies a bounded set of ECO resizes,
#  (b) reads back the INCREMENTAL slacks (only the affected cone recomputed),
#  (c) wipes ALL cached timing with `delays_invalid` and recomputes FULL from
#      scratch (same edited netlist), then
#  (d) asserts the incremental WNS / TNS / every per-endpoint slack are
#      BIT-FOR-BIT identical to the full re-run -- an incremental engine that
#      gives different numbers than full STA is a bug.
#  (e) asserts only the affected cone (< all endpoints) is flagged in-cone.
#  (f) asserts report_checks (the default full-STA path) output is UNCHANGED
#      by running report_incremental_sta (flag-off / additive guarantee).
#
# Float values are platform/delay-calc dependent, so they are NOT echoed; the
# test asserts the invariants in Tcl and prints only deterministic PASS/FAIL
# booleans.
source "helpers.tcl"
source "incremental_sta_helpers.tcl"
sta::set_thread_count 1

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def cppr.def
create_clock -name clk -period 0.3 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 0.05 [get_ports {in1 in2}]
set_output_delay -clock clk 0.05 [get_ports out1]

# Establish baseline full timing.
set base_wns [sta::worst_slack_cmd "max"]

# ECO edit: two resizes in the data cone.
replace_cell buf1 BUF_X2
replace_cell buf2 BUF_X2

lassign [inc_vs_full_oracle [list buf1 buf2]] w t e a n
puts "WNS inc==full: $w"
puts "TNS inc==full: $t"
puts "per-endpoint inc==full: $e"
puts "affected endpoints < total: [expr {$a < $n}]"
puts "affected endpoints >= 1: [expr {$a >= 1}]"

# (f) Additive guarantee: running report_incremental_sta must NOT perturb the
# default report_checks (full-STA) output. Both captures are taken on the SAME
# (edited) netlist; the only difference is whether report_incremental_sta ran
# in between.
with_output_to_variable gba_before {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
with_output_to_variable inc_report {
  report_incremental_sta -cells {buf1 buf2} -setup
}
with_output_to_variable gba_after {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
puts "report_checks unchanged by report_incremental_sta: \
[expr {$gba_before eq $gba_after}]"

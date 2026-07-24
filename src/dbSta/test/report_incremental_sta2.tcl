# report_incremental_sta: incremental STA == full STA after an ECO edit
# (additive, report-only).  Design 2: gcd (aocv_derate.def), a real ~570-cell
# Nangate45 netlist with parasitics and many timing endpoints.
#
# Second design for the correctness oracle (see report_incremental_sta.tcl for
# the full background). We resize two inverters on internal logic and assert
# the incremental slacks equal a full from-scratch re-run, and that only the
# affected fanout cone (a small fraction of all endpoints) is flagged in-cone.
source "helpers.tcl"
source "incremental_sta_helpers.tcl"
sta::set_thread_count 1

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Establish baseline full timing.
set base_wns [sta::worst_slack_cmd "max"]

# ECO edit: upsize two inverters on internal logic.
replace_cell _445_ INV_X2
replace_cell _446_ INV_X2

lassign [inc_vs_full_oracle [list _445_ _446_]] w t e a n
puts "WNS inc==full: $w"
puts "TNS inc==full: $t"
puts "per-endpoint inc==full: $e"
puts "affected endpoints < total: [expr {$a < $n}]"
puts "affected endpoints >= 1: [expr {$a >= 1}]"

# (f) Additive guarantee: report_incremental_sta must NOT perturb the default
# report_checks (full-STA) output. Both captures are on the SAME edited
# netlist; the only difference is whether report_incremental_sta ran between.
with_output_to_variable gba_before {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
with_output_to_variable inc_report {
  report_incremental_sta -cells {_445_ _446_} -setup
}
with_output_to_variable gba_after {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
puts "report_checks unchanged by report_incremental_sta: \
[expr {$gba_before eq $gba_after}]"

# report_cppr: Clock Reconvergence Pessimism Removal (report-only).
#
# Design (cppr.def, mirrors search_crpr.v): a reconvergent clock tree
#   clk -> ckbuf1 -> clk_buf1 -> {reg1/CK, ckbuf2 -> clk_buf2 -> reg2/CK}
# so the reg1 -> reg2 check shares the clk -> ckbuf1 clock segment. Under
# OCV derate that common segment is double-counted, and CPPR credits the
# pessimism back.
#
# Verifies:
#  (a) the report runs and produces the expected column header,
#  (b) the reg1 -> reg2 endpoint (reg2/D) gets a POSITIVE CPPR credit and
#      its common clock pin is the shared branch point ckbuf1/Z,
#  (c) CPPR slack == raw slack + credit, and credit >= 0 on every check
#      (CPPR only relaxes; raw slack is the pessimistic number),
#  (d) the credit equals OpenSTA's own "clock reconvergence pessimism"
#      value from report_checks (authoritative cross-check),
#  (e) report_checks (GBA) output is UNCHANGED by running report_cppr
#      (additive / report-only guarantee).
#
# Float values are platform/delay-calc dependent, so they are NOT echoed to
# stdout; the test asserts the invariants in Tcl and prints only
# deterministic PASS/FAIL booleans + the column header.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def cppr.def

create_clock -name clk -period 10 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 1.0 [get_ports in1]
set_input_delay -clock clk 1.0 [get_ports in2]
set_output_delay -clock clk 2.0 [get_ports out1]

# OCV analysis + derate so CRPR is active in the GBA engine.
set_operating_conditions -analysis_type on_chip_variation
set_timing_derate -late 1.05
set_timing_derate -early 0.95

# --- (e) Capture GBA report_checks output BEFORE running report_cppr.
with_output_to_variable gba_before {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}

# --- (a) Human-readable report -> capture, echo only the header line.
with_output_to_variable report {
  report_cppr -max_paths 10
}
foreach rline [split $report "\n"] {
  if { [string match "Endpoint*Raw slack*CPPR slack*Credit*Common clock pin" \
          $rline] } {
    puts "column header: $rline"
  }
}

# --- (b)(c) Machine-readable verification.
set lines [sta::cppr_report_lines 10 "max"]
set n_paths [llength $lines]

set all_credit_nonneg 1
set all_sum_ok 1
set reg2_credit 0.0
set reg2_common ""
set reg2_seen 0
foreach line $lines {
  # endpoint raw cppr credit common_pin
  set endpoint [lindex $line 0]
  set raw      [lindex $line 1]
  set cppr     [lindex $line 2]
  set credit   [lindex $line 3]
  set common   [lindex $line 4]
  if { $credit < -1e-15 } {
    set all_credit_nonneg 0
    puts "VIOLATION: $endpoint credit=$credit < 0"
  }
  if { abs(($raw + $credit) - $cppr) > 1e-15 } {
    set all_sum_ok 0
    puts "VIOLATION: $endpoint raw+credit != cppr"
  }
  if { $endpoint eq "reg2/D" } {
    set reg2_seen 1
    set reg2_credit $credit
    set reg2_common $common
  }
}

puts "paths found > 0: [expr {$n_paths > 0}]"
puts "reg2/D present: $reg2_seen"
puts "reg2/D credit > 0 (pessimism removed): [expr {$reg2_credit > 1e-15}]"
puts "reg2/D common pin is ckbuf1/Z: [expr {$reg2_common eq {ckbuf1/Z}}]"
puts "all credits non-negative: $all_credit_nonneg"
puts "cppr == raw + credit on all checks: $all_sum_ok"

# --- (d) Credit must equal OpenSTA's own CRPR value from report_checks.
# Parse the "clock reconvergence pessimism" delay column for reg1->reg2.
with_output_to_variable rc {
  report_checks -from reg1/CK -to reg2/D -path_delay max \
    -format full_clock_expanded -digits 4
}
set sta_crpr "NA"
foreach rline [split $rc "\n"] {
  if { [string match "*clock reconvergence pessimism*" $rline] } {
    set sta_crpr [lindex $rline 0]
  }
}
# reg2_credit is in seconds; report_checks prints ns with 4 digits.
set credit_ns [expr { $reg2_credit * 1e9 }]
puts "credit matches report_checks crpr: \
[expr { $sta_crpr ne {NA} && abs($credit_ns - $sta_crpr) < 5e-5 }]"

# --- (e) GBA report_checks output must be identical after report_cppr.
with_output_to_variable gba_after {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
puts "gba report_checks unchanged: [expr {$gba_before eq $gba_after}]"

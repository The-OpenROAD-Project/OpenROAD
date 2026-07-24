# report_pba_slack: Path-Based Analysis pessimism-recovery (first slice).
#
# Verifies:
#  (a) the report runs and produces the expected column header,
#  (b) PBA slack >= GBA slack on every reported path (PBA recovers
#      pessimism; recovered delta is non-negative),
#  (c) report_checks (GBA) output is UNCHANGED by running the PBA command
#      (additive-only guarantee).
#
# Float values are intentionally NOT echoed to stdout (they are platform/
# delay-calc dependent); the test asserts the invariants in Tcl and prints
# only deterministic PASS/FAIL booleans + counts + the column header.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def pba_gcd.def

create_clock [get_ports clk] -name core_clock -period 0.485

# --- (c) Capture GBA report_checks output BEFORE running any PBA command.
# Use with_output_to_variable so the (float-laden, platform-dependent)
# report_checks dump does not reach stdout / the .ok file.
with_output_to_variable gba_before {
  report_checks -path_delay max -fields {slew cap} -digits 4
}

# --- (a) Human-readable report -> capture report-stream output, echo only
# the header line so the .ok file stays deterministic (no float values).
with_output_to_variable report {
  report_pba_slack -max_paths 10
}
foreach rline [split $report "\n"] {
  if { [string match "Endpoint*GBA slack*PBA slack*Recovered*Gates" $rline] } {
    puts "column header: $rline"
  }
}

# --- (b) Machine-readable verification of the PBA >= GBA invariant.
set lines [sta::pba_slack_report_lines 10 "max"]
set n_paths [llength $lines]

set all_ok 1
set min_recovered 1e30
set total_gate_stages 0
foreach line $lines {
  lassign $line endpoint gba pba recovered gates
  if { $recovered < -1e-12 } {
    set all_ok 0
    puts "VIOLATION: $endpoint recovered=$recovered < 0"
  }
  if { [expr {$pba - $gba}] < -1e-12 } {
    set all_ok 0
    puts "VIOLATION: $endpoint pba=$pba < gba=$gba"
  }
  if { $recovered < $min_recovered } {
    set min_recovered $recovered
  }
  set total_gate_stages [expr {$total_gate_stages + $gates}]
}

puts "paths found > 0: [expr {$n_paths > 0}]"
puts "invariant pba>=gba on all paths: $all_ok"
puts "min recovered non-negative: [expr {$min_recovered >= -1e-12}]"
puts "gate stages evaluated > 0: [expr {$total_gate_stages > 0}]"

# --- (c) GBA report_checks output must be identical after the PBA command.
with_output_to_variable gba_after {
  report_checks -path_delay max -fields {slew cap} -digits 4
}
puts "gba report_checks unchanged: [expr {$gba_before eq $gba_after}]"

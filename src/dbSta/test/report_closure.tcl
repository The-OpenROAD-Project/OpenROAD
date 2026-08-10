# report_closure: unified PBA + CPPR pessimism-recovery closure verdict.
#
# Design (closure.def): a reconvergent clock tree
#   clk -> ckbuf1 -> {reg1/CK, reg3/CK, ckbuf2 -> reg2/CK}
# plus two data cones, engineered so that under OCV derate the unified
# closure report classifies one endpoint into EACH of the three classes:
#
#   * reg2/D  -- CPPR-recoverable ARTIFACT. The reg1 -> reg2 check shares the
#                clk -> ckbuf1 clock segment; under derate that common segment
#                is double-counted. The period is tuned so reg2/D fails under
#                the raw (no-CRPR) baseline but the CPPR common-path credit
#                alone clears it. Its short single-buffer data path yields ~0
#                PBA recovery, so it is cleared by CPPR alone.
#
#   * reg3/D  -- PBA-recoverable ARTIFACT. A long AND2 chain (g1..g7) whose
#                second inputs are tied to a slow buffer chain (sbuf0..sbuf4)
#                creates gate-slew pessimism that PBA recovers. reg3 is clocked
#                off the SAME clk_buf1 leaf as its launch (reg1), so it gets
#                ~0 CPPR credit. The in3 input delay is tuned so reg3/D fails
#                under the raw baseline but the PBA gate-slew recovery alone
#                clears it -- cleared by PBA, not CPPR.
#
#   * out1    -- GENUINE violation. The reg2 -> out1 output path with a large
#                set_output_delay fails by ~2 ns and has neither a
#                reconvergent-clock credit nor enough gate stages to recover,
#                so it stays failing after BOTH recoveries.
#
#   * reg1/D  -- a positive-slack (not raw-failing) control endpoint.
#
# Verifies:
#  (a) the unified closure report runs and produces the expected headers,
#  (b) the composition invariants hold on every endpoint:
#        recovered_slack == raw_slack + cppr_credit + pba_recovered,
#        cppr_credit >= 0, pba_recovered >= 0
#      (the report COMPOSES the existing PBA + CPPR paths; recovery is
#       monotone, so recovered_slack >= raw_slack),
#  (c) the closure verdict is exactly "4 endpoints, 3 raw-failing, 1 cleared
#      by CPPR, 1 cleared by PBA, 0 by both, 1 genuine",
#  (d) each engineered endpoint lands in the right class with the right
#      mechanism label: reg2/D -> CPPR artifact, reg3/D -> PBA artifact,
#      out1 -> GENUINE, reg1/D -> not raw-failing,
#  (e) report_checks (GBA) output is UNCHANGED by running report_closure
#      (additive / report-only guarantee).
#
# Float values are platform/delay-calc dependent, so they are NOT echoed to
# stdout; the test asserts the invariants in Tcl and prints only deterministic
# PASS/FAIL booleans + counts + the column headers.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def closure.def

# Period tuned so reg2/D's raw failure is cleared by the CPPR common-path
# credit alone (same 0.123 ns reconvergent point as the cppr regression).
create_clock -name clk -period 0.123 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 0.0 [get_ports in1]
set_input_delay -clock clk 0.0 [get_ports in2]
# in3 delay positions reg3/D just inside the PBA gate-slew recovery window:
# raw slightly negative, recovered slightly positive.
set_input_delay -clock clk -0.2008 [get_ports in3]
# Large output delay makes out1 a wide, unrecoverable (genuine) violation.
set_output_delay -clock clk 2.0 [get_ports out1]

# OCV analysis + derate so CRPR is active in the GBA engine.
set_operating_conditions -analysis_type on_chip_variation
set_timing_derate -late 1.05
set_timing_derate -early 0.95

# --- (e) Capture GBA report_checks output BEFORE running any closure command.
with_output_to_variable gba_before {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}

# --- (a) Human-readable report -> echo only the deterministic header lines.
with_output_to_variable closure_report {
  report_closure -all -max_paths 20
}
foreach rline [split $closure_report "\n"] {
  if { [string match "Unified pessimism-recovery closure -- setup (max)" \
          $rline] } {
    puts "report header: [string trim $rline]"
  }
  if { [string match "Endpoint*Raw slack*Recovered*CPPR*PBA*Verdict" $rline] } {
    puts "column header: $rline"
  }
}

# --- (b) Machine-readable verification of the composition invariants.
# line = endpoint raw gba recovered cppr_credit pba_recovered raw_v genuine \
#        cleared_by
set lines [sta::closure_report_lines 20 "max"]
set n_eps [llength $lines]

set all_credit_nonneg 1
set all_recover_nonneg 1
set all_sum_ok 1
foreach line $lines {
  lassign $line endpoint raw gba recovered cppr_cr pba_rec rawv genuine cleared
  if { $cppr_cr < -1e-15 } {
    set all_credit_nonneg 0
    puts "VIOLATION: $endpoint cppr_credit=$cppr_cr < 0"
  }
  if { $pba_rec < -1e-15 } {
    set all_recover_nonneg 0
    puts "VIOLATION: $endpoint pba_recovered=$pba_rec < 0"
  }
  # recovered_slack == raw + cppr_credit + pba_recovered (composition).
  if { abs(($raw + $cppr_cr + $pba_rec) - $recovered) > 1e-15 } {
    set all_sum_ok 0
    puts "VIOLATION: $endpoint raw+cppr+pba != recovered"
  }
}
puts "endpoints found == 4: [expr {$n_eps == 4}]"
puts "all cppr credits non-negative: $all_credit_nonneg"
puts "all pba recovery non-negative: $all_recover_nonneg"
puts "recovered == raw + cppr + pba on all endpoints: $all_sum_ok"

# --- (c) Closure verdict: deterministic counts from the engineered design.
# summary = "<eps> <raw_failing> <cleared_cppr> <cleared_pba> <cleared_both> \
#            <genuine>"
set summary [sta::closure_summary 20 "max"]
puts "closure summary: $summary"
lassign $summary s_eps s_raw s_cppr s_pba s_both s_genuine
puts "closure: 3 raw-failing endpoints: [expr {$s_raw == 3}]"
puts "closure: exactly 1 cleared by CPPR: [expr {$s_cppr == 1}]"
puts "closure: exactly 1 cleared by PBA: [expr {$s_pba == 1}]"
puts "closure: 0 cleared by CPPR+PBA: [expr {$s_both == 0}]"
puts "closure: exactly 1 genuine violation: [expr {$s_genuine == 1}]"
# Every raw failure is accounted for: cleared (any mechanism) + genuine.
puts "closure: cleared + genuine == raw-failing: \
[expr {$s_cppr + $s_pba + $s_both + $s_genuine == $s_raw}]"

# --- (d) Each engineered endpoint lands in the right class with the right
# mechanism label.
array set CLASS {}
foreach line $lines {
  lassign $line endpoint raw gba recovered cppr_cr pba_rec rawv genuine cleared
  set CLASS($endpoint) [list $rawv $genuine $cleared]
}
lassign $CLASS(reg2/D) r2_rawv r2_gen r2_cleared
lassign $CLASS(reg3/D) r3_rawv r3_gen r3_cleared
lassign $CLASS(out1)   o1_rawv o1_gen o1_cleared
lassign $CLASS(reg1/D) r1_rawv r1_gen r1_cleared
set reg2_cppr_artifact [expr {$r2_rawv == 1 && $r2_gen == 0 && $r2_cleared eq "CPPR"}]
set reg3_pba_artifact [expr {$r3_rawv == 1 && $r3_gen == 0 && $r3_cleared eq "PBA"}]
set out1_genuine [expr {$o1_rawv == 1 && $o1_gen == 1}]
set reg1_not_failing [expr {$r1_rawv == 0 && $r1_gen == 0}]
puts "reg2/D is a CPPR artifact (raw-failing, not genuine, cleared by CPPR): $reg2_cppr_artifact"
puts "reg3/D is a PBA artifact (raw-failing, not genuine, cleared by PBA): $reg3_pba_artifact"
puts "out1 is a genuine violation (raw-failing and still genuine): $out1_genuine"
puts "reg1/D is not raw-failing (positive-slack control): $reg1_not_failing"

# --- (e) GBA report_checks output must be identical after the closure command.
with_output_to_variable gba_after {
  report_checks -path_delay max -format full_clock_expanded -digits 4
}
puts "gba report_checks unchanged: [expr {$gba_before eq $gba_after}]"

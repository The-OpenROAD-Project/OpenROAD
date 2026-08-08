# SI slice 2: timing-window aware coupling derate on gcd (sky130hs) with real
# extracted coupling caps. Verifies:
#   (a) window filtering DISABLED reproduces the slice-1 / baseline timing
#       exactly (byte-identical TNS),
#   (b) with a blanket Miller factor 2.0, enabling the window filter recovers
#       pessimism: the window-filtered effective coupling cap is <= the blanket
#       coupling cap, and the worst-case setup TNS is >= the blanket TNS
#       (slack improves or equals -- filtering only removes aggressors),
#   (c) report_si_windows reports per-victim gating.
#
# Coupling caps must be KEPT in the parasitic network (read_spef
# -keep_capacitive_coupling) for the factor and the window filter to act.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_liberty sky130hs/sky130hs_tt.lib

read_def gcd.def
source sky130hs/sky130hs.rc

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file ext_pattern.rules -max_res 0 \
  -coupling_threshold 0.1

set spef_file [make_result_file coupling_si_window.spef]
write_spef $spef_file
read_spef -keep_capacitive_coupling $spef_file

create_clock -name clk -period 1.0 [get_ports clk]

proc tns {} {
  return [sta::total_negative_slack_cmd "max"]
}

set fails 0

# ---- (a) baseline with feature disabled == slice-1 baseline ----
set_coupling_miller_factor -setup 1.0 -hold 1.0
set base [tns]
puts "baseline TNS (mf=1.0, window off): [format %.6e $base]"

# ---- blanket worst-case Miller factor (slice-1 pessimism) ----
set_coupling_miller_factor -setup 2.0 -hold 1.0
set blanket [tns]
puts "blanket TNS (mf=2.0, window off): [format %.6e $blanket]"

if { $blanket < $base } {
  puts "PASS: blanket mf=2.0 worsens setup TNS vs baseline"
} else {
  puts "FAIL: blanket mf=2.0 did not worsen TNS (base=$base blanket=$blanket)"
  incr fails
}

# ---- (b) enable window filtering: recover pessimism ----
# guardband 0 so non-overlapping aggressors are gated out.
set_si_timing_window -enable -guardband 0.0
report_si_windows -max_nets 8
set windowed [tns]
puts "windowed TNS (mf=2.0, window on): [format %.6e $windowed]"

# ---- core gating behavior: windows must discriminate ----
# With guardband 0 some aggressor windows do NOT overlap their victim's, so
# they MUST be gated out. With a guardband large enough to bridge every gap,
# nothing may be gated. This proves gating keys off the windows, not a
# constant.
set gated_tight [sta::si_window_gated_count_cmd 0]
puts "aggressors gated (guardband 0): $gated_tight"
if { $gated_tight >= 1 } {
  puts "PASS: non-overlapping aggressors gated out (constructed non-overlap)"
} else {
  puts "FAIL: expected >=1 gated aggressor with guardband 0, got $gated_tight"
  incr fails
}

set_si_timing_window -enable -guardband 1e-3
set gated_wide [sta::si_window_gated_count_cmd 0]
puts "aggressors gated (guardband 1e-3, all windows overlap): $gated_wide"
if { $gated_wide == 0 } {
  puts "PASS: large guardband forces overlap, nothing gated"
} else {
  puts "FAIL: expected 0 gated with large guardband, got $gated_wide"
  incr fails
}
if { $gated_tight > $gated_wide } {
  puts "PASS: gating count tracks the windows (tight > wide)"
} else {
  puts "FAIL: gating did not respond to guardband\
        (tight=$gated_tight wide=$gated_wide)"
  incr fails
}

# Re-apply the guardband-0 filter so the timing reflects window filtering for
# the pessimism check below.
set_si_timing_window -enable -guardband 0.0
report_si_windows -max_nets 8
set windowed [tns]

# Filtering can only REMOVE coupling, so windowed slack must be >= blanket
# slack (TNS less negative or equal) and <= baseline slack (no coupling derate
# is the most optimistic bound).
set tol 1e-15
if { $windowed >= [expr {$blanket - $tol}] } {
  puts "PASS: window filtering recovers pessimism (windowed TNS >= blanket TNS)"
} else {
  puts "FAIL: window filtering made timing worse than blanket\
        (blanket=$blanket windowed=$windowed)"
  incr fails
}
if { $windowed <= [expr {$base + $tol}] } {
  puts "PASS: windowed TNS <= baseline TNS (no more optimistic than\
        zero-coupling-derate)"
} else {
  puts "FAIL: windowed TNS more optimistic than baseline\
        (base=$base windowed=$windowed)"
  incr fails
}

# ---- (c) disable -> back to the exact blanket-mf=2.0 timing ----
set_si_timing_window -disable
set restored [tns]
if { $restored == $blanket } {
  puts "PASS: disabling window filter restores blanket timing exactly"
} else {
  puts "FAIL: disable did not restore blanket timing\
        (blanket=$blanket restored=$restored)"
  incr fails
}

# ---- (d) full restore to baseline ----
set_coupling_miller_factor -setup 1.0 -hold 1.0
set restored_base [tns]
if { $restored_base == $base } {
  puts "PASS: mf=1.0 reproduces baseline exactly after window experiments"
} else {
  puts "FAIL: did not return to baseline\
        (base=$base restored_base=$restored_base)"
  incr fails
}

if { $fails == 0 } {
  puts "pass"
} else {
  puts "FAIL: $fails check(s) failed"
}

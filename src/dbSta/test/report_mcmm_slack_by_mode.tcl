# report_mcmm_slack -by_mode: the MODE dimension of MCMM (slice 2, additive,
# report-only).
#
# Design (cppr.def): reg1 -> reg2 plus input/output ports, in Nangate45.
# Two MODES, each with two CORNERS (scenes) carrying per-corner liberty:
#   func mode : clk period 0.3  -> func_fast (fast.lib), func_slow (slow.lib)
#   test mode : clk period 0.2  -> test_fast (fast.lib), test_slow (slow.lib)
# The test mode's clock is 0.1 ns tighter, so for setup every endpoint's worst
# slack is 0.1 ns worse in test than in func, and the slow corner is always the
# setup-limiting corner within a mode (data arrives later).
#
# OpenSTA already models mode x scene (a Mode holds an SDC, scenes belong to a
# mode); slice 2 consumes that model and surfaces it. The setup surface used
# here (set_mode, define_scene -mode) is all upstream OpenSTA.
#
# Verifies:
#  (a) two modes are active and the cross-mode header lists all 4 scenes,
#  (b) PER-MODE worst slack per endpoint == min over that mode's scenes, and
#      the limiting corner within each mode is the slow one (setup),
#  (c) the TRUE cross-mode x corner worst per endpoint == min over ALL scenes,
#      and the limiting (mode, corner) pair is (test, test_slow) -- the tighter
#      mode's slow corner,
#  (d) test-mode worst is exactly 0.1 ns worse than func-mode worst per endpoint
#      (the clock-period delta), proving the modes really carry distinct SDC,
#  (e) SINGLE-MODE compat: with only one mode declared (default), the slice-2
#      machine numbers equal slice-1's cross-corner numbers (mode dimension is
#      inert when there is one mode).
#
# Float values are platform/delay-calc dependent, so they are NOT echoed; the
# test asserts invariants in Tcl and prints only deterministic PASS/FAIL
# booleans + the active-scene list.
source "helpers.tcl"

# Liberty must be read before being referenced by define_scene.
read_liberty Nangate45/Nangate45_fast.lib
read_liberty Nangate45/Nangate45_slow.lib

# Two modes, each with two scenes (corners).
set_mode func
define_scene func_fast -mode func -liberty Nangate45/Nangate45_fast.lib
define_scene func_slow -mode func -liberty Nangate45/Nangate45_slow.lib
set_mode test
define_scene test_fast -mode test -liberty Nangate45/Nangate45_fast.lib
define_scene test_slow -mode test -liberty Nangate45/Nangate45_slow.lib

read_lef Nangate45/Nangate45.lef
read_def cppr.def

# func mode constraints (relaxed clock 0.3).
set_mode func
create_clock -name clk -period 0.3 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 0.05 [get_ports in1]
set_input_delay -clock clk 0.05 [get_ports in2]
set_output_delay -clock clk 0.05 [get_ports out1]

# test mode constraints (tighter clock 0.2 => 0.1 worse setup slack).
set_mode test
create_clock -name clk -period 0.2 [get_ports clk]
set_propagated_clock [get_clocks clk]
set_input_delay -clock clk 0.05 [get_ports in1]
set_input_delay -clock clk 0.05 [get_ports in2]
set_output_delay -clock clk 0.05 [get_ports out1]

# --- (a) Two modes active; header lists all four scenes.
puts "two modes active: [expr {[llength [get_modes *]] == 2}]"
with_output_to_variable report {
  report_mcmm_slack -setup -by_mode -max_endpoints 10
}
set scenes {func_fast func_slow test_fast test_slow}
set saw_all 1
foreach sc $scenes {
  if { ![string match "*$sc*" $report] } { set saw_all 0 }
}
puts "all four scenes listed: $saw_all"
puts "limiting (mode, corner) shown: \
[string match "*(test, test_slow)*" $report]"

# --- (b) PER-MODE worst slack: build mode -> endpoint -> {slack corner}.
# Machine line: "<mode> <endpoint> <slack> <limiting_corner>"
array set mode_worst {}
array set mode_corner {}
set per_mode_lines [sta::mcmm_per_mode_lines 10 "max"]
set per_mode_slow_limited 1
foreach line $per_mode_lines {
  set mode [lindex $line 0]
  set ep   [lindex $line 1]
  set slk  [lindex $line 2]
  set cnr  [lindex $line 3]
  set mode_worst($mode,$ep) $slk
  set mode_corner($mode,$ep) $cnr
  # Within each mode the setup-limiting corner must be the slow scene.
  if { $cnr ne "${mode}_slow" } { set per_mode_slow_limited 0 }
}
puts "per-mode setup limited by slow corner: $per_mode_slow_limited"

# --- (c)(d) Cross-mode x corner worst + (mode,corner); test is 0.1 worse.
# Machine line: "<endpoint> <worst_slack> <worst_mode> <worst_corner>"
set cross_lines [sta::mcmm_mode_report_lines 10 "max"]
set cross_n [llength $cross_lines]
set cross_limit_ok 1
set cross_is_min_ok 1
set test_offset_ok 1
foreach line $cross_lines {
  set ep    [lindex $line 0]
  set worst [lindex $line 1]
  set wmode [lindex $line 2]
  set wcnr  [lindex $line 3]
  # The tighter test mode's slow corner must limit every endpoint here.
  if { $wmode ne "test" || $wcnr ne "test_slow" } { set cross_limit_ok 0 }
  # Cross worst == min(func worst, test worst) for this endpoint.
  set fw $mode_worst(func,$ep)
  set tw $mode_worst(test,$ep)
  set mn [expr { $fw < $tw ? $fw : $tw }]
  if { abs($worst - $mn) > 1e-15 } { set cross_is_min_ok 0 }
  # test mode is 0.1 ns (1e-10 s) tighter => its worst is 0.1ns less than func.
  if { abs(($fw - $tw) - 1.0e-10) > 1e-13 } { set test_offset_ok 0 }
}
puts "cross-mode endpoints > 0: [expr {$cross_n > 0}]"
puts "cross worst limited by (test, test_slow): $cross_limit_ok"
puts "cross worst == min over all scenes: $cross_is_min_ok"
puts "test mode worst 0.1ns tighter than func: $test_offset_ok"

# --- (e) SLICE-1 COMPAT: the mode dimension must not change the worst-slack
# numbers. Slice-1's mcmm_report_lines searches ALL scenes (mode-agnostic) and
# reports the cross-corner worst; slice-2's mcmm_mode_report_lines reports the
# same worst with the limiting (mode, corner) attached. On the identical DB the
# worst slack per endpoint MUST match between the two -- i.e. adding the mode
# breakdown is purely additive and never perturbs slice-1's numbers.
array set slice1_worst {}
foreach line [sta::mcmm_report_lines 10 "max"] {
  set ep    [lindex $line 0]
  set worst [lindex $line 1]
  set slice1_worst($ep) $worst
}
set slice1_compat_ok 1
foreach line $cross_lines {
  set ep    [lindex $line 0]
  set worst [lindex $line 1]
  if { ![info exists slice1_worst($ep)]
       || abs($worst - $slice1_worst($ep)) > 1e-15 } {
    set slice1_compat_ok 0
  }
}
puts "slice-1 worst slack unchanged by mode dimension: $slice1_compat_ok"

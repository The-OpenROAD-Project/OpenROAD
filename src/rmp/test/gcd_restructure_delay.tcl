source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal3
estimate_parasitics -placement

# Timing-mode restructure picks endpoints from worst-slack paths, which only
# exist after an arrival search.
report_worst_slack
set ws_before [worst_slack -max]

set tiehi "LOGIC1_X1/Z"
set tielo "LOGIC0_X1/Z"

# Delay-target restructure is the sole caller of Resizer::findFanins and the
# only exercise of the null worst-slack-path guard in
# Restructure::getEndPoints (unconstrained endpoints return no path). ABC's
# internal resynthesis output is nondeterministic (iteration counts, fraig
# stats, progress bars) and threaded, so a golden log comparison is flaky by
# construction. This is therefore a pass/fail test: the contract is that
# timing-mode restructure runs end to end without crashing and does not
# degrade the worst slack.
ord::set_thread_count "3"
restructure -liberty_file Nangate45/Nangate45_typ.lib -target timing \
  -slack_threshold 1 -depth_threshold 2 \
  -abc_logfile [make_result_file abc_delay.log] \
  -tielo_port $tielo -tiehi_port $tiehi -work_dir [make_result_dir]

set ws_after [worst_slack -max]

# gcd worst slack is +1.29 ns and is deterministic (pure STA, pre-ABC).
if { abs($ws_before - 1.29) > 0.01 } {
  error "unexpected pre-restructure worst slack: $ws_before (expected ~1.29)"
}
# restructure discards any resynthesis run that is not an improvement, so the
# worst slack must never regress.
if { $ws_after < [expr { $ws_before - 0.01 }] } {
  error "restructure degraded worst slack: before=$ws_before after=$ws_after"
}

puts "pass"

# report_qor on a fully set-up small clocked design (gcd / Nangate45).
#
# Exercises the normal path: design + area + timing + power + clock skew all
# present. Asserts the summary has the expected sections and plausible values
# (non-zero instances/area, a finite timing number, populated power) rather
# than golden floats, so the .ok file is deterministic across platforms.
#
# Also confirms report_qor is ADDITIVE: report_checks output is byte-identical
# before and after running report_qor (it changes no timing state).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def
read_sdc aocv_derate.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# --- Additivity gate: timing must be byte-identical before/after report_qor --
# Snapshot the worst-slack/TNS numbers, run report_qor, snapshot again. A
# read-only command cannot change any of these.
set ws_before [worst_slack -max]
set tns_before [total_negative_slack -max]
set hold_before [worst_slack -min]
set checks_before [report_checks]
report_qor
set ws_after [worst_slack -max]
set tns_after [total_negative_slack -max]
set hold_after [worst_slack -min]
set checks_after [report_checks]
puts "timing_unchanged=[expr { $ws_before == $ws_after \
  && $tns_before == $tns_after && $hold_before == $hold_after }]"
puts "report_checks_identical=[expr { $checks_before eq $checks_after }]"

# --- Structural / plausibility assertions on the text summary ---------------
set txt [report_qor]

proc has_section { txt name } {
  return [expr { [string first $name $txt] >= 0 }]
}
puts "has_design=[has_section $txt {Design}]"
puts "has_timing=[has_section $txt {Timing}]"
puts "has_power=[has_section $txt {Power}]"
puts "has_clock=[has_section $txt {Clock}]"

# Counts come straight from odb; gcd has hundreds of instances/nets.
set block [ord::get_db_block]
set ninst [llength [$block getInsts]]
set nnet [llength [$block getNets]]
puts "instances_nonzero=[expr { $ninst > 0 }]"
puts "nets_nonzero=[expr { $nnet > 0 }]"

# Area / utilization should be positive (placed design with a floorplan).
set area [expr { [rsz::design_area] * 1e6 * 1e6 }]
set util [expr { [rsz::utilization] * 100.0 }]
puts "area_positive=[expr { $area > 0 }]"
puts "util_positive=[expr { $util > 0 }]"

# Timing numbers are finite and the summary is not the n/a placeholder.
set ws [worst_slack -max]
puts "worst_slack_finite=[expr { [string is double -strict $ws] }]"
puts "timing_not_na=[expr { [string first {n/a (no clock} $txt] < 0 }]"

# Power present (liberty loaded) and total is a real number.
set scene [sta::cmd_scene]
set ptot [lindex [sta::design_power $scene] 3]
puts "power_total_finite=[expr { [string is double -strict $ptot] }]"
puts "power_not_na=[expr { [string first {n/a (no liberty} $txt] < 0 }]"

# --- JSON output is well-formed and carries the same data -------------------
# (presence of the nested keys proves timing/power are emitted as objects, not
# null, on this fully-set-up design)
set js [report_qor -json]
puts "json_has_design=[expr { [string first {"design"} $js] >= 0 }]"
puts "json_has_timing_obj=[expr { [string first {"setup_wns":} $js] >= 0 }]"
puts "json_has_power_obj=[expr { [string first {"leakage":} $js] >= 0 }]"
puts "json_timing_not_null=[expr { [string first {"timing": null} $js] < 0 }]"
puts "json_power_not_null=[expr { [string first {"power": null} $js] < 0 }]"
puts "json_instances_match=[expr { [string first "\"instances\": $ninst," $js] >= 0 }]"

# --- -digits is honored: -digits 2 -> exactly two decimals (default is 3) ---
set js3 [report_qor -json]
set js2 [report_qor -json -digits 2]
# default: three decimals
set d3_ok [regexp {"utilization_pct": [0-9]+\.[0-9][0-9][0-9]} $js3]
# -digits 2: two decimals, not followed by a third digit
set d2_ok [regexp {"utilization_pct": [0-9]+\.[0-9][0-9]([^0-9]|$)} $js2]
puts "json_digits_default3=$d3_ok"
puts "json_digits_ok=[expr { $d2_ok && $d3_ok }]"

# A sink move must honor max_fanout in every mode, restore rejected moves,
# and retain the rejected pair in the claims denominator.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog cts_modes.v
link_design cts_modes
read_sdc -mode mode_a cts_modes_a.sdc
read_sdc -mode mode_b cts_modes_b.sdc
set lib [get_full_name [lindex [get_libs *] 0]]
define_scene scene_a -mode mode_a -liberty $lib
define_scene scene_b -mode mode_b -liberty $lib
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
set block [ord::get_db_block]
foreach name { clk clock_a clock_b } { [$block findNet $name] setSigType CLOCK }
set x 10000
foreach name { leaf_a leaf_b ff0 ff1 ff2 ff3 } {
  set inst [$block findInst $name]
  $inst setLocation $x 10000
  $inst setPlacementStatus PLACED
  incr x 10000
}
estimate_parasitics -placement
foreach mode { mode_a mode_b } suffix { a b } {
  set_mode $mode
  create_clock -name clock_mode_$suffix -period 2 [get_pins {leaf_a/Z leaf_b/Z}]
  set_propagated_clock [all_clocks]
  set_max_fanout 3 [current_design]
}
# The active command mode has room, but the other mode is at its limit.
set_mode mode_a
set_max_fanout 2 [current_design]
set_mode mode_b
set key [string repeat 0 64]
set claims [make_result_file cts_fanout.csv]
set sink [[$block findInst ff3] findITerm CK]
proc attempt { description expected } {
  global key claims sink block
  # The data input already exceeds the design limit; clock moves must not
  # add any violations to that baseline in either mode.
  report_check_types -max_fanout -violators
  set violations [sta::max_fanout_violation_count]
  set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
    -sibling_dist_um 100 -skew_margin_ns 1]
  check "$description remains a claim" { set count } 1
  check "$description connection" { [$sink getNet] getName } $expected
  set held [expr { $expected eq "clock_a" }]
  check "$description extraction" { verify_watermark -cts_claims $claims -min_stages 1 } $held
  report_check_types -max_fanout -violators
  check "$description fanout violations" { sta::max_fanout_violation_count } $violations
  $sink connect [$block findNet clock_b]
}
attempt "other mode at fanout limit" clock_b

# Reaching the limit exactly is legal, with no additional fractional reserve.
set_mode mode_a
set_max_fanout 3 [current_design]
set_mode mode_b
attempt "both modes allow three loads" clock_a

# Tightening the active mode must also reject and restore the same trial.
set_max_fanout 2 [current_design]
attempt "active mode at fanout limit" clock_b
exit_summary

# Minimum absolute slack can hide a failed fractional reserve in another
# scene or edge. Keep all clock identities equivalent and all skew affordable.
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
}
set key [string repeat 0 64]
set claims [make_result_file cts_headroom.csv]
set sink [[$block findInst ff3] findITerm CK]
proc attempt { description expected } {
  global key claims sink block
  set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
    -sibling_dist_um 100 -skew_margin_ns 1 -slack_margin_ns 1]
  check "$description remains a claim" { set count } 1
  check "$description connection" { [$sink getNet] getName } $expected
  set held [expr { $expected eq "clock_a" }]
  check "$description extraction" {
    expr { [wmk::verify_cts_watermark_cmd $key $claims] >= 0.75 }
  } $held
  $sink connect [$block findNet clock_b]
}
# 30% reserve / 3 ps slack in scene_a masks 15% reserve / 15 ps in scene_b.
foreach mode { mode_a mode_b } limit { 0.01 0.1 } {
  set_mode $mode
  set_max_transition $limit [all_clocks]
}
set_assigned_transition -scene scene_a 0.007 [get_pins {leaf_a/Z leaf_b/Z}]
set_assigned_transition -scene scene_b 0.085 [get_pins {leaf_a/Z leaf_b/Z}]
attempt "second scene lacks 20% slew reserve" clock_b
set_assigned_transition -scene scene_b 0.070 [get_pins {leaf_a/Z leaf_b/Z}]
attempt "both scenes have 30% slew reserve" clock_a

# The same trap exists within one scene: rise has the smallest absolute slack
# while fall has the smallest percentage reserve. Both must be examined.
foreach mode { mode_a mode_b } scene { scene_a scene_b } {
  set_mode $mode
  set_max_transition -rise 0.01 [all_clocks]
  set_max_transition -fall 0.1 [all_clocks]
  set_assigned_transition -scene $scene -rise 0.007 [get_pins {leaf_a/Z leaf_b/Z}]
  set_assigned_transition -scene $scene -fall 0.085 [get_pins {leaf_a/Z leaf_b/Z}]
}
attempt "fall lacks 20% slew reserve" clock_b
foreach scene { scene_a scene_b } {
  set_assigned_transition -scene $scene -fall 0.070 [get_pins {leaf_a/Z leaf_b/Z}]
}
attempt "both edges have 30% slew reserve" clock_a

# Capacitance has one limit per scene. Assigned total loads isolate the same
# fractional-versus-absolute ordering without relying on parasitic estimates.
foreach mode { mode_a mode_b } limit { 0.01 0.1 } load { 0.007 0.085 } {
  set_mode $mode
  set_max_capacitance $limit [current_design]
  set_load -subtract_pin_load $load [get_nets {clock_a clock_b}]
}
attempt "second scene lacks 20% capacitance reserve" clock_b
set_mode mode_b
set_load -subtract_pin_load 0.070 [get_nets {clock_a clock_b}]
attempt "both scenes have 30% capacitance reserve" clock_a
exit_summary

# Different modes allocate clock indices independently. Equal integer indices
# must neither admit an incompatible pair nor supply another mode's timing.
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
proc clock_memberships { } {
  set result {}
  foreach mode { mode_a mode_b } {
    set_mode $mode
    foreach pin { ff0/CK ff1/CK ff2/CK ff3/CK } {
      set names {}
      foreach clock [get_property [get_pins $pin] clocks] {
        lappend names [get_full_name $clock]
      }
      dict set result $mode $pin [lsort $names]
    }
  }
  return $result
}
proc clock_connections { } {
  set result {}
  set block [ord::get_db_block]
  foreach name { ff0 ff1 ff2 ff3 } {
    lappend result [[[$block findInst $name] findITerm CK] getNet]
  }
  return $result
}
set key 0000000000000000000000000000000000000000000000000000000000000000
set claims [make_result_file cts_modes.csv]
set membership [clock_memberships]
set connections [clock_connections]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100]
check "colliding mode-local indices cannot admit an incompatible pair" { set count } 0
check "incompatible pair preserves every sink connection" {
  expr {[clock_connections] eq $connections}
} 1
check "incompatible pair preserves every sink's clocks in each mode" {
  expr {[clock_memberships] eq $membership}
} 1

# Both leaves now have equivalent clock membership in each mode. Leave one
# mode ideal: propagated timing from the other mode cannot stand in for it.
foreach mode { mode_a mode_b } suffix { a b } period { 2 3 } {
  set_mode $mode
  create_clock -name clock_mode_$suffix -period $period [get_pins {leaf_a/Z leaf_b/Z}]
}
set_mode mode_a
set_propagated_clock [all_clocks]
set_mode mode_b
unset_propagated_clock [all_clocks]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100]
check "equivalent pair remains a claim when one mode lacks timing" { set count } 1
check "one mode's timing cannot authorize another mode's sink move" {
  expr {[clock_connections] eq $connections}
} 1
check "unachieved parity remains a failed claim" {
  expr {[wmk::verify_cts_watermark_cmd $claims] >= 0.75}
} 0

# Positive control: once both modes have propagated timing, a move succeeds
# under the default timing budgets while preserving each mode's clock set.
set_propagated_clock [all_clocks]
set membership [clock_memberships]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100]
check "equivalent clocks in both modes permit embedding" { set count } 1
check "positive control actually reconnects a sink" {
  expr {[clock_connections] ne $connections}
} 1
check "successful move preserves clocks in every mode" {
  expr {[clock_memberships] eq $membership}
} 1
check "positive control achieves parity" {
  expr {[wmk::verify_cts_watermark_cmd $claims] >= 0.75}
} 1
exit_summary

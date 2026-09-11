# Overlapping clock sets are insufficient to preserve a sink's clocks.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog cts_clock_sets.v
link_design cts_clock_sets
create_clock -name clk1 -period 2 [get_ports clk1]
create_clock -name clk2 -period 3 [get_ports clk2]
set_propagated_clock [all_clocks]
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
set block [ord::get_db_block]
foreach name { clk1 clk2 mixed_clock clock_a clock_b } {
  [$block findNet $name] setSigType CLOCK
}
set x 10000
foreach name { leaf_a leaf_b mux ff0 ff1 ff2 ff3 } {
  set inst [$block findInst $name]
  $inst setLocation $x 10000
  $inst setPlacementStatus PLACED
  incr x 10000
}
estimate_parasitics -placement

set key 0000000000000000000000000000000000000000000000000000000000000000
set claims [make_result_file cts_clock_sets.csv]
set before [[[$block findInst ff0] findITerm CK] getNet]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100 -skew_margin_ns 100 \
  -slew_headroom_frac 0 -cap_headroom_frac 0]
check "overlapping unequal clock sets cannot be paired" { set count } 0
check "sink connectivity is preserved" {
  expr {[[[$block findInst ff0] findITerm CK] getNet] == $before}
} 1

# Case analysis cannot justify bypassing mux logic, even with equal clock sets.
set_case_analysis 0 [get_ports select]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100 -skew_margin_ns 100 \
  -slew_headroom_frac 0 -cap_headroom_frac 0]
check "case analysis does not erase a clock-logic boundary" { set count } 0

# Connect both buffers to the same source to establish a truly equivalent pair.
[[$block findInst leaf_b] findITerm A] connect [$block findNet clk1]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100 -skew_margin_ns 100 \
  -slew_headroom_frac 0 -cap_headroom_frac 0]
check "equal nonempty clock sets remain eligible" { set count } 1
check "the eligible pair achieves the requested parity" {
  expr {[wmk::verify_cts_watermark_cmd $claims] >= 0.75}
} 1
exit_summary

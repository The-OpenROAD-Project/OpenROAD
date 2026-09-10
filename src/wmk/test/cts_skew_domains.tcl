# A high-skew clock must not mask degradation in another clock domain.
source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog cts_skew_domains.v
link_design cts_skew_domains
create_clock -name clk1 -period 2 [get_ports clk1]
create_clock -name clk2 -period 3 [get_ports clk2]
set_propagated_clock [all_clocks]
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
set block [ord::get_db_block]
foreach name { clk1 clk2 clock_a clock_b } {
  [$block findNet $name] setSigType CLOCK
}
set x 10000
foreach name { leaf_a leaf_b ff0 ff1 ff2 ff3 high0 high1 } {
  set inst [$block findInst $name]
  $inst setLocation $x 10000
  $inst setPlacementStatus PLACED
  incr x 10000
}
estimate_parasitics -placement
set_assigned_delay -net -from [get_ports clk2] -to [get_pins high0/CK] 0
set_assigned_delay -net -from [get_ports clk2] -to [get_pins high1/CK] 1
# clk2 has 1 ns of skew, larger than any change to the small clk1 tree.
# It must not supply the baseline for edits to clk1.
set key 0000000000000000000000000000000000000000000000000000000000000000
set claims [make_result_file cts_skew_domains.csv]
set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100 -skew_margin_ns 0 -slew_headroom_frac 0 -cap_headroom_frac 0]
check "the rejected pair is still claimed" { set count } 1
check "an unrelated clock cannot hide skew degradation" {
  verify_watermark -cts_claims $claims -min_stages 1
} 0

# With enough budget, the same keyed move succeeds. This also proves that
# the first call had a movable sink and usable timing.
cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100 -skew_margin_ns 0.5 -slew_headroom_frac 0 -cap_headroom_frac 0
check "the move succeeds with a sufficient clock-specific budget" {
  verify_watermark -cts_claims $claims -min_stages 1
} 1
exit_summary

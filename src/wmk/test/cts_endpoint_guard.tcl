# Clock latency spread alone cannot establish setup/hold path safety.
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

# Fix the extremes in the affected clock itself. A move can now worsen a
# setup/hold path while leaving the measured clock-latency spread unchanged.
foreach name { high0 high1 } {
  [[$block findInst $name] findITerm CK] connect [$block findNet clk1]
}
set_assigned_delay -net -from [get_ports clk1] -to [get_pins high0/CK] 0
set_assigned_delay -net -from [get_ports clk1] -to [get_pins high1/CK] 1
set key 0000000000000000000000000000000000000000000000000000000000000000
set claims [make_result_file cts_endpoint_guard.csv]
set count [tee -variable output [list cts_watermark -key_hex $key -claims_file $claims \
  -num_pairs 1 -sibling_dist_um 100 -skew_margin_ns 0 \
  -slew_headroom_frac 0 -cap_headroom_frac 0]]
check "timing-rejected pairs remain claimed" { set count } 1
check "latency spread permits this move" { string match {*0 rejected on skew*} $output } 1
check "the endpoint guard rejects it" { string match {*1 on setup/hold timing*} $output } 1
check "the rejected mark fails verification" {
  verify_watermark -cts_claims $claims -min_stages 1
} 0

# A positive control ensures that eligibility or another guard is not the cause.
cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
  -sibling_dist_um 100 -skew_margin_ns 0.5 -slew_headroom_frac 0 -cap_headroom_frac 0
check "the move succeeds with enough timing budget" {
  verify_watermark -cts_claims $claims -min_stages 1
} 1
exit_summary

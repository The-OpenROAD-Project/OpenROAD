# Clock names cannot prove that a reconnect preserves gating or polarity.
source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog cts_clock_logic.v
link_design cts_clock_logic
create_clock -name clk -period 2 [get_ports clk]
set_propagated_clock [all_clocks]
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
set block [ord::get_db_block]
foreach name { clk gated_clock inverted_clock clock_a clock_b } {
  [$block findNet $name] setSigType CLOCK
}
set x 10000
foreach name { leaf_a leaf_b gate ff0 ff1 ff2 ff3 invert } {
  set inst [$block findInst $name]
  $inst setLocation $x 10000
  $inst setPlacementStatus PLACED
  incr x 10000
}
estimate_parasitics -placement

proc clock_connections { } {
  set connections {}
  foreach name { ff0 ff1 ff2 ff3 } {
    lappend connections [[[ord::get_db_block] findInst $name] findITerm CK]
  }
  return [lmap pin $connections { [$pin getNet] getName }]
}
set before [clock_connections]
set key 0000000000000000000000000000000000000000000000000000000000000000
set claims [make_result_file cts_clock_logic.csv]
set command [list cts_watermark -key_hex $key -claims_file $claims \
  -num_pairs 1 -sibling_dist_um 100]
check "a gate cannot be bypassed with equal named clocks" { {*}$command } 0
check "all sequential clock connections are preserved" { clock_connections } $before
set_case_analysis 1 [get_ports enable_clock]
check "case analysis cannot erase a gating boundary" { {*}$command } 0

# Moving within the same gated branch is safe and must remain possible.
[[$block findInst leaf_a] findITerm A] connect [$block findNet gated_clock]
set count [tee -variable output $command]
check "buffers below the same gate can be paired" { set count } 1
check "the positive control actually moves a sink" {
  string match {*1 sinks moved*} $output
} 1
check "the gated mark round-trips" {
  expr {[wmk::verify_cts_watermark_cmd $key $claims] >= 0.75}
} 1

# A single-output clock gate is not a valid carrier, whatever parity the key
# asks of it.  The claim must still be one the key derives, so pair the gate
# with whichever name makes it the target.
set peer leaf_a
while { [wmk::cts_target_lcb_cmd $key gate $peer] ne "gate" } {
  set peer "${peer}_"
}
set fh [open $claims w]
puts $fh "pair_idx,pair_key,target_lcb,other_lcb,target_bit,final_bit,skipped_reason"
puts $fh "0,gate+$peer,gate,$peer,[wmk::cts_target_bit_cmd $key gate $peer],0,"
close $fh
check "verification refuses a non-buffer carrier" {
  expr {[wmk::verify_cts_watermark_cmd $key $claims] >= 0.75}
} 0

# Named clocks still match through an inverter, but their edges do not.
[[$block findInst leaf_a] findITerm A] connect [$block findNet inverted_clock]
set before [clock_connections]
check "opposite polarities cannot be paired" { {*}$command } 0
check "polarity rejection preserves every sink" { clock_connections } $before
exit_summary

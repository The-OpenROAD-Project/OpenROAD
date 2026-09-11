# Protected source or destination nets must not leave a sink disconnected.
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
set key 0000000000000000000000000000000000000000000000000000000000000000
proc connections { } {
  set result [dict create]
  foreach inst [[ord::get_db_block] getInsts] {
    foreach pin [$inst getITerms] {
      dict set result [$pin getId] [list [$pin getNet] [$pin getModNet]]
    }
  }
  return $result
}
set claims [make_result_file cts_protected.csv]
set before [connections]
foreach protected { clock_a clock_b } {
  set_dont_touch [get_nets $protected]
  set count [cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
    -sibling_dist_um 100]
  check "$protected: the refused pair remains claimed" { set count } 1
  check "$protected: all connections are preserved" { connections } $before
  check "$protected: an unsuccessful mark does not verify" {
    expr {[wmk::verify_cts_watermark_cmd $claims] >= 0.75}
  } 0
  unset_dont_touch [get_nets $protected]
}
# Positive control: these exact buffers and timing allow a move once unprotected.
cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 -sibling_dist_um 100
check "unprotected embedding moves a sink" { expr { [connections] ne $before } } 1
check "the unprotected mark verifies" {
  expr {[wmk::verify_cts_watermark_cmd $claims] >= 0.75}
} 1
exit_summary

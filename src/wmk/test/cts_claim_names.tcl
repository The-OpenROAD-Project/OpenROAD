# Reject unsupported names before moving any sink or overwriting claims.
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
proc contents { path } {
  set stream [open $path r]
  set result [read $stream]
  close $stream
  return $result
}
set claims [make_result_file cts_claim_names.csv]
set leaf [$block findInst leaf_b]
foreach unsupported { zzleaf,b "zzleaf\nnext" " zzleaf" "zzleaf " } {
  $leaf rename $unsupported
  set before [connections]
  set stream [open $claims w]
  puts -nonewline $stream "previous claim file"
  close $stream
  set failed [catch {
    cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 -sibling_dist_um 100
  } message]
  check "unsupported name fails explicitly" { set failed } 1
  check "the error identifies the claim format" { string match {*WMK-0112*} $message } 1
  check "all connections are preserved" { connections } $before
  check "the existing claim file is preserved" { contents $claims } "previous claim file"
}
$leaf rename leaf_b
cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 -sibling_dist_um 100
check "a supported name can be embedded and verified" {
  expr {[wmk::verify_cts_watermark_cmd $key $claims] >= 0.75}
} 1
exit_summary

# Flat reconnect/rollback cannot update module ports: reject hierarchy up front.
source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog cts_hierarchy.v
link_design nested -hier
create_clock -name clk1 -period 2 [get_ports clk1]
set_propagated_clock [all_clocks]
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
set block [ord::get_db_block]
foreach name { clk1 clock_a clock_b } {
  [$block findNet $name] setSigType CLOCK
}
set x 10000
foreach name { leaf_a leaf_b bank_a/ff0 bank_a/ff1 bank_b/ff0 bank_b/ff1 } {
  set inst [$block findInst $name]
  $inst setLocation $x 10000
  $inst setPlacementStatus PLACED
  incr x 10000
}
estimate_parasitics -placement
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
set pin [[$block findInst bank_b/ff1] findITerm CK]
check "the fixture has real hierarchical sink connectivity" {
  expr { [$pin getModNet] ne "NULL" }
} 1
set before [connections]
set verilog_before [make_result_file cts_hierarchy.before.v]
set verilog_after [make_result_file cts_hierarchy.after.v]
write_verilog $verilog_before
set key 0000000000000000000000000000000000000000000000000000000000000000
set claims [make_result_file cts_hierarchy.csv]
# Zero budget formerly corrupted hierarchy on ordinary skew rollback; the
# default budget corrupted it on an accepted move. Both must fail before edits.
foreach margin { 0 0.020 } {
  set stream [open $claims w]
  puts -nonewline $stream "previous claim file"
  close $stream
  set failed [catch {
    cts_watermark -key_hex $key -claims_file $claims -num_pairs 1 \
      -sibling_dist_um 100 -skew_margin_ns $margin
  } message]
  check "$margin: hierarchical embedding fails explicitly" { set failed } 1
  check "$margin: the error explains the flat-design requirement" {
    string match {*WMK-0111*} $message
  } 1
  check "$margin: all flat and module connections survive" { connections } $before
  write_verilog $verilog_after
  check "$margin: the exported netlist is unchanged" {
    contents $verilog_after
  } [contents $verilog_before]
  check "$margin: the existing claim file is preserved" {
    contents $claims
  } "previous claim file"
}
# Verification is read-only and remains supported for a hierarchical design.
set stream [open $claims w]
puts $stream "target_lcb,target_bit,skipped_reason"
puts $stream "leaf_a,0,"
close $stream
check "hierarchical claims can still be verified" {
  expr {[wmk::verify_cts_watermark_cmd $claims] >= 0.75}
} 1
exit_summary

# Count-aware ownership decisions on real ODB instances and clock buffers.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
set block [ord::get_db_block]
set placement [make_result_file evidence_placement.csv]
set cts [make_result_file evidence_cts.csv]

proc placement_claims { path count held } {
  set stream [open $path w]
  puts $stream "kind,A_name,B_name,target_bit,skipped_reason"
  set instances [[ord::get_db_block] getInsts]
  for { set i 0 } { $i < $count } { incr i } {
    set a [lindex $instances [expr { 2 * $i }]]
    set b [lindex $instances [expr { 2 * $i + 1 }]]
    set bit [expr { [[$a getBBox] xMin] < [[$b getBBox] xMin] ? 0 : 1 }]
    if { $i >= $held } { set bit [expr { 1 - $bit }] }
    puts $stream "pair,[$a getName],[$b getName],$bit,"
  }
  close $stream
}

# Give CTS a real sequential-clock carrier without requiring a whole CTS run.
set master [[ord::get_db] findMaster CLKBUF_X3]
set leaf [odb::dbInst_create $block $master evidence_leaf]
set net [odb::dbNet_create $block evidence_clock]
$net setSigType CLOCK
[$leaf findITerm Z] connect $net
foreach inst [$block getInsts] {
  set pin [$inst findITerm CK]
  if { $pin ne "NULL" } {
    $pin connect $net
    break
  }
}
set stream [open $cts w]
puts $stream "target_lcb,target_bit,skipped_reason"
puts $stream "evidence_leaf,1,"
close $stream
placement_claims $placement 1 1
lassign [wmk::verify_placement_claims_cmd $placement] count held probability
check "placement counts survive the Tcl boundary" { list $count $held $probability } {1 1 0.5}
lassign [wmk::verify_cts_claims_cmd $cts] count held probability
check "CTS counts survive the Tcl boundary" { list $count $held $probability } {1 1 0.5}
check "one matching bit per stage is insufficient by default" {
  verify_watermark -placement_claims $placement -cts_claims $cts
} 0
check "selecting fewer stages does not waive the evidence threshold" {
  verify_watermark -placement_claims $placement -min_stages 1
} 0
check "an explicitly relaxed evidence budget is honored" {
  verify_watermark -placement_claims $placement -cts_claims $cts -claim_alpha 0.5
} 1

foreach count { 13 14 } expected { 0 1 } {
  placement_claims $placement $count $count
  check "$count perfect bits at the default evidence threshold" {
    verify_watermark -placement_claims $placement -min_stages 1
  } $expected
}
placement_claims $placement 64 48
check "48 of 64 matches satisfies both default thresholds" {
  verify_watermark -placement_claims $placement -min_stages 1
} 1
check "the extraction threshold still applies" {
  verify_watermark -placement_claims $placement -min_stages 1 -tau 0.8
} 0
check "a stricter evidence budget is respected" {
  verify_watermark -placement_claims $placement -min_stages 1 -claim_alpha 1e-6
} 0
placement_claims $placement 64 32
check "chance-level unmarked evidence fails even with tau zero" {
  verify_watermark -placement_claims $placement -min_stages 1 -tau 0
} 0
foreach alpha { NaN Inf -Inf 0 -1 1 1.1 invalid } {
  check "invalid claim alpha $alpha is rejected" {
    catch { verify_watermark -placement_claims $placement -claim_alpha $alpha }
  } 1
}
exit_summary

# Count-aware ownership decisions on real ODB instances and clock buffers.
#
# Every claim written here carries the bit the key derives for it, as a claim
# file must; what varies is how many of those bits the design happens to show.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
set block [ord::get_db_block]
set key [string repeat 0 64]
set placement [make_result_file evidence_placement.csv]
set cts [make_result_file evidence_cts.csv]

# Sort the design's cells into pairs whose keyed bit it already shows and pairs
# whose bit it does not, so a claim file with any number of held claims can be
# written from them.
set holding {}
set failing {}
set instances [$block getInsts]
for { set i 0 } { $i + 1 < [llength $instances] } { incr i 2 } {
  set a [lindex $instances $i]
  set b [lindex $instances [expr { $i + 1 }]]
  if { [$a getName] > [$b getName] } {
    lassign [list $b $a] a b
  }
  set bit [wmk::placement_target_bit_cmd $key [$a getName] [$b getName]]
  set observed [expr { [[$a getBBox] xMin] < [[$b getBBox] xMin] ? 0 : 1 }]
  set row "pair,[$a getName],[$b getName],$bit,"
  if { $observed == $bit } {
    lappend holding $row
  } else {
    lappend failing $row
  }
}
check "the design offers enough of both kinds of pair" {
  expr { [llength $holding] >= 64 && [llength $failing] >= 32 }
} 1

proc placement_claims { path count held } {
  global holding failing
  set stream [open $path w]
  puts $stream "kind,A_name,B_name,target_bit,skipped_reason"
  foreach row [lrange $holding 0 [expr { $held - 1 }]] {
    puts $stream $row
  }
  foreach row [lrange $failing 0 [expr { $count - $held - 1 }]] {
    puts $stream $row
  }
  close $stream
}

# Give CTS a real sequential-clock carrier without requiring a whole CTS run:
# a pair of leaf buffers, the target of which drives as many flops as the
# keyed parity asks for.
set master [[ord::get_db] findMaster CLKBUF_X3]
foreach name { evidence_leaf_a evidence_leaf_b } {
  set leaf [odb::dbInst_create $block $master $name]
  set net [odb::dbNet_create $block ${name}_clock]
  $net setSigType CLOCK
  [$leaf findITerm Z] connect $net
}
set target [wmk::cts_target_lcb_cmd $key evidence_leaf_a evidence_leaf_b]
set other [expr { $target eq "evidence_leaf_a" ? "evidence_leaf_b" : "evidence_leaf_a" }]
set target_bit [wmk::cts_target_bit_cmd $key evidence_leaf_a evidence_leaf_b]
if { $target_bit == 1 } {
  foreach inst [$block getInsts] {
    set pin [$inst findITerm CK]
    if { $pin ne "NULL" } {
      $pin connect [$block findNet ${target}_clock]
      break
    }
  }
}
set stream [open $cts w]
puts $stream "target_lcb,other_lcb,target_bit,skipped_reason"
puts $stream "$target,$other,$target_bit,"
close $stream
placement_claims $placement 1 1
lassign [wmk::verify_placement_claims_cmd $key $placement] count held probability
check "placement counts survive the Tcl boundary" { list $count $held $probability } {1 1 0.5}
lassign [wmk::verify_cts_claims_cmd $key $cts] count held probability
check "CTS counts survive the Tcl boundary" { list $count $held $probability } {1 1 0.5}
set keyed [list -placement_key_hex $key -cts_key_hex $key]
check "one matching bit per stage is insufficient by default" {
  verify_watermark -placement_claims $placement -cts_claims $cts {*}$keyed
} 0
check "selecting fewer stages does not waive the evidence threshold" {
  verify_watermark -placement_claims $placement {*}$keyed -min_stages 1
} 0
check "an explicitly relaxed evidence budget is honored" {
  verify_watermark -placement_claims $placement -cts_claims $cts {*}$keyed -claim_alpha 0.5
} 1

foreach count { 13 14 } expected { 0 1 } {
  placement_claims $placement $count $count
  check "$count perfect bits at the default evidence threshold" {
    verify_watermark -placement_claims $placement {*}$keyed -min_stages 1
  } $expected
}
placement_claims $placement 64 48
check "48 of 64 matches satisfies both default thresholds" {
  verify_watermark -placement_claims $placement {*}$keyed -min_stages 1
} 1
check "the extraction threshold still applies" {
  verify_watermark -placement_claims $placement {*}$keyed -min_stages 1 -tau 0.8
} 0
check "a stricter evidence budget is respected" {
  verify_watermark -placement_claims $placement {*}$keyed -min_stages 1 -claim_alpha 1e-6
} 0
placement_claims $placement 64 32
check "chance-level unmarked evidence fails even with tau zero" {
  verify_watermark -placement_claims $placement {*}$keyed -min_stages 1 -tau 0
} 0
foreach alpha { NaN Inf -Inf 0 -1 1 1.1 invalid } {
  check "invalid claim alpha $alpha is rejected" {
    catch { verify_watermark -placement_claims $placement {*}$keyed -claim_alpha $alpha }
  } 1
}
check "a claims file cannot be checked without its key" {
  catch { verify_watermark -placement_claims $placement -min_stages 1 } message
} 1
check "the error asks for the key" { string match {*WMK-0140*} $message } 1
exit_summary

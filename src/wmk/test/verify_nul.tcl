# One inserted NUL must not turn an absent instance into a real ODB name.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

# The key wm_place_claims.csv was written with.
set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee

set stream [open wm_place_claims.csv r]
set lines [split [string trim [read $stream]] \n]
close $stream
proc write_binary_claims { path lines } {
  set stream [open $path w]
  fconfigure $stream -translation binary
  puts $stream [join $lines \n]
  close $stream
}
set block [ord::get_db_block]
# Row 6 is the failing claim; replace its second cell with one that does not
# exist, recording the bit the key derives for the new pair so that only the
# missing instance separates the claim from a held one.  Names are written in
# the order the key sees them, so the bit is the pair's own.
set fields [split [lindex $lines 5] ,]
set a_name [lindex $fields 2]
set ax [[[$block findInst $a_name] getBBox] xMin]
set peer ""
foreach inst [$block getInsts] {
  if { [[$inst getBBox] xMin] > $ax } {
    set peer [$inst getName]
    break
  }
}
if { $peer eq "" } { error "fixture requires a cell to A's right" }
lassign [lsort [list $a_name "${peer}_missing"]] first second
lset fields 2 $first
lset fields 3 $second
lset fields 4 [wmk::placement_target_bit_cmd $key $first $second]
set missing_field [expr { $first eq $a_name ? 3 : 2 }]
lset lines 5 [join $fields ,]
set claims [make_result_file verify_nul.csv]
write_binary_claims $claims $lines
check "absent instance makes the fifth claim fail" {
  verify_watermark -placement_claims $claims -placement_key_hex $key -tau 0.9 \
    -min_stages 1
} 0
lset fields $missing_field "${peer}\x00_missing"
lset lines 5 [join $fields ,]
write_binary_claims $claims $lines
check "a NUL cannot change the failed claim into an ownership pass" {
  catch {
    tee -variable message [list verify_watermark -placement_claims $claims \
      -placement_key_hex $key -tau 0.9 -min_stages 1]
  }
} 1
check "the diagnostic identifies the file and offending row" {
  expr {[string first $claims $message] >= 0 && [string match {*line 6: NUL byte*} $message]}
} 1

set cts_claims [make_result_file verify_nul_cts.csv]
write_binary_claims $cts_claims [list "target_lcb,other_lcb,target_bit,skipped_reason" \
  "${peer}\x00_missing,peer,0,"]
check "CTS verification also rejects NUL names" {
  catch {
    tee -variable message [list verify_watermark -cts_claims $cts_claims \
      -cts_key_hex $key -min_stages 1]
  }
} 1
check "CTS diagnostic identifies the offending row" {
  string match {*line 2: NUL byte*} $message
} 1
exit_summary

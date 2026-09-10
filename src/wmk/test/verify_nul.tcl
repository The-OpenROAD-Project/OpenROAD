# One inserted NUL must not turn an absent instance into a real ODB name.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def
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
set ax [[[$block findInst _284_] getBBox] xMin]
set peer ""
foreach inst [$block getInsts] {
  if { [[$inst getBBox] xMin] > $ax } {
    set peer [$inst getName]
    break
  }
}
if { $peer eq "" } { error "fixture requires a cell to A's right" }
set fields [split [lindex $lines 5] ,]
lset fields 6 "${peer}_missing"
lset lines 5 [join $fields ,]
set claims [make_result_file verify_nul.csv]
write_binary_claims $claims $lines
check "absent instance makes the fifth claim fail" {
  verify_watermark -placement_claims $claims -tau 0.9 -min_stages 1
} 0
lset fields 6 "${peer}\x00_missing"
lset lines 5 [join $fields ,]
write_binary_claims $claims $lines
check "a NUL cannot change the failed claim into an ownership pass" {
  catch {
    tee -variable message [list verify_watermark -placement_claims $claims -tau 0.9 -min_stages 1]
  }
} 1
check "the diagnostic identifies the file and offending row" {
  expr {[string first $claims $message] >= 0 && [string match {*line 6: NUL byte*} $message]}
} 1

set cts_claims [make_result_file verify_nul_cts.csv]
write_binary_claims $cts_claims [list "target_lcb,target_bit,skipped_reason" \
  "${peer}\x00_missing,0,"]
check "CTS verification also rejects NUL names" {
  catch {
    tee -variable message [list verify_watermark -cts_claims $cts_claims -min_stages 1]
  }
} 1
check "CTS diagnostic identifies the offending row" {
  string match {*line 2: NUL byte*} $message
} 1
exit_summary

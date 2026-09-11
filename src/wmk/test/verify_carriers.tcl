# Malformed carrier identities must fail before any ownership scoring.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
set path [make_result_file verify_carriers.csv]
foreach { stage header rows expected } {
  placement "kind,A_name,B_name,target_bit,skipped_reason"
    "pair,_276_,_276_,1," "two different instances"
  placement "kind,A_name,B_name,target_bit,skipped_reason"
    "pair,_276_,_277_,0,\npair,_276_,_277_,0," "duplicate placement pair"
  placement "kind,A_name,B_name,target_bit,skipped_reason"
    "pair,_276_,_277_,0,\npair,_277_,_276_,1," "duplicate placement pair"
  cts "target_lcb,target_bit,skipped_reason"
    "leaf,0,\nleaf,0," "duplicate CTS target"
  cts "target_lcb,target_bit,skipped_reason"
    "leaf,0,\nleaf,1,already_satisfied" "duplicate CTS target"
} {
  set stream [open $path w]
  puts $stream $header
  puts $stream [subst -nocommands -novariables $rows]
  close $stream
  set failed [catch {
    tee -variable diagnostic [list verify_watermark -${stage}_claims $path -min_stages 1]
  } message]
  check "$stage invalid carriers produce an error" { set failed } 1
  check "$stage identifies the malformed carrier" {
    string match "*$expected*" $diagnostic
  } 1
  check "$stage error identifies the row" { regexp {line [23]:} $diagnostic } 1
  check "$stage never scores a malformed file" { string match {*claims hold*} $diagnostic } 0
}
exit_summary

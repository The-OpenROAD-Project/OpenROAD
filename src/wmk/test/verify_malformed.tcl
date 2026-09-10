# A malformed failing claim must abort verification, not raise its score.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

check "valid evidence fails the strict threshold" {
  verify_watermark -placement_claims wm_place_claims.csv -tau 0.9 -min_stages 1
} 0

set fh [open wm_place_claims.csv r]
set lines [split [string trim [read $fh]] \n]
close $fh
set damaged [make_result_file verify_malformed.csv]
# Row 6 is the one failing claim. Removing its final empty field used to
# silently discard it and turn 4/5 evidence into a 4/4 ownership pass.
lset lines 5 [string trimright [lindex $lines 5] ,]
set fh [open $damaged w]
puts $fh [join $lines \n]
close $fh
set failed [catch {
  tee -variable message [list verify_watermark -placement_claims $damaged -tau 0.9 -min_stages 1]
}]
check "malformed evidence is refused" { set failed } 1
check "the error identifies the damaged row" {
  string match {*line 6: expected*fields*} $message
} 1
exit_summary

# A MINIMUMCUT rule that requires a single cut is always satisfied, since
# every via has at least one cut. drt ignored the cut count and flagged (and
# avoided) every via landing on a shape covered by such a rule. With the rule
# width at the layer's minimum width no metal4 shape escapes the rule, so the
# via onto pin b could never be legal.
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def single_cut_minimumcut.def

# MINIMUMCUT 1 WIDTH 0.14 on metal4 (0.14um is the metal4 minimum width)
set layer [[ord::get_db_tech] findLayer metal4]
set rule [odb::dbTechMinCutRule_create $layer]
$rule setMinimumCuts 1 280 0 0

make_tracks
global_route
set drc_file [make_result_file single_cut_minimumcut.drc]
detailed_route -verbose 0 -output_drc $drc_file

set violations 0
set fh [open $drc_file]
foreach line [split [read $fh] "\n"] {
  if { [string match "violation type:*" $line] } {
    incr violations
  }
}
close $fh
puts "violations: $violations"

# Count via MASK color tokens in a DEF file. A via MASK statement in DEF 5.8
# is "MASK <top><cut><bottom> <viaName>" -- MASK, digits, then a via name
# token. This pattern counts ONLY cut/via mask tokens, never metal-segment
# ones (which are "MASK <d> ( ...").
proc count_via_mask_tokens { def_path } {
  set fh [open $def_path r]
  set data [read $fh]
  close $fh
  return [regexp -all {MASK [0-9]+ [A-Za-z]} $data]
}

source "helpers.tcl"
# Multi-patterning slice 6: conflict-graph coloring extended to CUT/VIA layers
# -- the UNCOLORABLE (odd-cycle) case.
#
# Tech: the "via" CUT layer is double-patterned (MASK 2), cut width 0.15um,
# same-mask cut spacing 0.17um. mask_solve_cut_oddcycle.def places 3 M1M2_PR
# vias in a triangle so that ALL three cut pairs are closer than the same-mask
# cut spacing -> the cut conflict graph is K3 (an odd cycle), which is NOT
# 2-colorable.
#
# The solver must REPORT the uncolorable cut conflict (>=1) and must NOT emit a
# (necessarily illegal) cut coloring for that component. A correct bounded
# solver that honestly reports the uncolorable case is the win; emitting an
# illegal coloring to force a pass would be wrong.

read_lef sky130hd/sky130hd_multi_patterned_cut.tlef
read_def mask_solve_cut_oddcycle.def

set_mask_aware_routing -enable -solve_coloring -solve_cut_coloring -num_color_masks 2
set rpt [make_result_file mask_solve_cut_oddcycle.rpt]
set uncolorable [drt::solve_mask_coloring -output_file $rpt]
puts "UNCOLORABLE: $uncolorable"

set out [make_result_file mask_solve_cut_oddcycle.def]
write_def $out
set via_masks [count_via_mask_tokens $out]
puts "VIA_MASK_TOKENS: $via_masks"

if { $uncolorable < 1 } {
  error "FAIL: odd cycle of cuts was not reported as uncolorable"
}
# The K3 cut component is the whole set of vias, so no cut may be colored.
if { $via_masks != 0 } {
  error "FAIL: solver emitted a cut coloring on an uncolorable cut graph"
}
puts "pass"

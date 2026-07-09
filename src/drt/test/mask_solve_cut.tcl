# Count via MASK color tokens in a DEF file. A via MASK statement in DEF 5.8
# is "MASK <top><cut><bottom> <viaName>" -- i.e. MASK, digits, then a via
# name token. Routed-segment MASK tokens are instead "MASK <d> ( ..." so this
# pattern counts ONLY cut/via mask tokens, never metal-segment ones.
proc count_via_mask_tokens { def_path } {
  set fh [open $def_path r]
  set data [read $fh]
  close $fh
  return [regexp -all {MASK [0-9]+ [A-Za-z]} $data]
}

source "helpers.tcl"
# Multi-patterning slice 6: conflict-graph legal coloring extended to CUT/VIA
# layers -- the 2-COLORABLE case.
#
# Tech: the "via" CUT layer is double-patterned (MASK 2), cut width 0.15um,
# same-mask cut spacing 0.17um. mask_solve_cut.def places 3 M1M2_PR vias at
# x = 1.0, 1.3, 1.6um (same y): adjacent cut pairs are closer than the
# same-mask cut spacing but the outer pair is not -> the cut conflict graph is
# a chain (0-1-2), which is bipartite and so 2-colorable. The solver must emit
# a legal 2-coloring of the cuts (colors 1,2,1) that check_mask_drc confirms
# has 0 same-mask cut-spacing violations.

read_lef sky130hd/sky130hd_multi_patterned_cut.tlef
read_def mask_solve_cut.def

# Gate: with the cut solver disabled (default), even with the metal solver on,
# solve_mask_coloring must touch NO cut layer (no cut MASK tokens emitted).
set_mask_aware_routing -enable -solve_coloring -num_color_masks 2
set off [make_result_file mask_solve_cut_off.def]
set u0 [drt::solve_mask_coloring -output_file /dev/null]
write_def $off
set masks_off [count_via_mask_tokens $off]
puts "CUTOFF_UNCOLORABLE: $u0"
puts "CUTOFF_VIA_MASK_TOKENS: $masks_off"
if { $masks_off != 0 } {
  error "FAIL: cut solver off but $masks_off via MASK token(s) emitted"
}

# Now enable cut coloring and solve.
set_mask_aware_routing -enable -solve_coloring -solve_cut_coloring -num_color_masks 2
set rpt [make_result_file mask_solve_cut.rpt]
set uncolorable [drt::solve_mask_coloring -output_file $rpt]
puts "UNCOLORABLE: $uncolorable"

set out [make_result_file mask_solve_cut.def]
write_def $out
set masks [count_via_mask_tokens $out]
puts "VIA_MASK_TOKENS: $masks"

set drc [make_result_file mask_solve_cut.maskdrc]
set viols [drt::check_mask_drc -output_file $drc]
puts "VIOLATIONS: $viols"

if { $uncolorable != 0 } {
  error "FAIL: 2-colorable cut layout reported $uncolorable uncolorable"
}
if { $masks == 0 } {
  error "FAIL: cut solver emitted no coloring for a colorable cut layout"
}
if { $viols != 0 } {
  error "FAIL: solved cut coloring is not legal ($viols mask violations)"
}
puts "pass"

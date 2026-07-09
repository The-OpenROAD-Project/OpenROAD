# Count routed segments carrying a MASK color token in a DEF file.
proc count_mask_tokens { def_path } {
  set fh [open $def_path r]
  set data [read $fh]
  close $fh
  return [regexp -all {MASK } $data]
}

source "helpers.tcl"
# Multi-patterning slice 5: conflict-graph legal 2-coloring solver --
# the UNCOLORABLE (odd-cycle) case.
#
# Multi-patterned tech: met1 is double-patterned (MASK 2), width 0.14um,
# same-mask spacing 0.14um. mask_solve_oddcycle.def has 3 met1 shapes arranged
# as a 2D triangle so that ALL three pairs are closer than the same-mask
# spacing -> the conflict graph is K3 (an odd cycle), which is NOT 2-colorable.
#
# The solver must REPORT the uncolorable conflict (>=1) and must NOT emit a
# (necessarily illegal) coloring for that component. A correct bounded solver
# that honestly reports the uncolorable case is the win; emitting an illegal
# coloring to force a pass would be wrong.

read_lef sky130hd/sky130hd_multi_patterned.tlef
read_def mask_solve_oddcycle.def

set_mask_aware_routing -enable -solve_coloring -num_color_masks 2
set rpt [make_result_file mask_solve_oddcycle.rpt]
set uncolorable [drt::solve_mask_coloring -output_file $rpt]
puts "UNCOLORABLE: $uncolorable"

set out [make_result_file mask_solve_oddcycle.def]
write_def $out
set masks [count_mask_tokens $out]
puts "MASK_TOKENS: $masks"

if { $uncolorable < 1 } {
  error "FAIL: odd cycle was not reported as uncolorable"
}
# The K3 component is the whole design, so no shape may be colored.
if { $masks != 0 } {
  error "FAIL: solver emitted a coloring on an uncolorable graph"
}
puts "pass"

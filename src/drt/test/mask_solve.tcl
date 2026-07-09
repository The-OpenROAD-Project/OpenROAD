# Count routed segments carrying a MASK color token in a DEF file.
proc count_mask_tokens { def_path } {
  set fh [open $def_path r]
  set data [read $fh]
  close $fh
  return [regexp -all {MASK } $data]
}

source "helpers.tcl"
# Multi-patterning slice 5: conflict-graph legal 2-coloring solver --
# the 2-COLORABLE case.
#
# Multi-patterned tech: met1 is double-patterned (MASK 2), width 0.14um,
# same-mask spacing 0.14um. mask_solve_ok.def has 3 parallel met1 wires where
# adjacent pairs are closer than the same-mask spacing but the outer pair is
# not -> the conflict graph is a chain (0-1-2), which is bipartite and so
# 2-colorable. The solver must emit a legal 2-coloring (colors 1,2,1) that
# check_mask_drc (slice 1) confirms has 0 same-mask violations.

read_lef sky130hd/sky130hd_multi_patterned.tlef
read_def mask_solve.def

# Gate: with the solver disabled (default), solve_mask_coloring must refuse.
set err [catch { drt::solve_mask_coloring -output_file /dev/null } msg]
if { $err == 0 } {
  error "solve_mask_coloring ran while solver gate disabled (gate failed)"
}
puts "GATE_OK: solver disabled by default"

set_mask_aware_routing -enable -solve_coloring -num_color_masks 2
set rpt [make_result_file mask_solve_ok.rpt]
set uncolorable [drt::solve_mask_coloring -output_file $rpt]
puts "UNCOLORABLE: $uncolorable"

set out [make_result_file mask_solve_ok.def]
write_def $out
set masks [count_mask_tokens $out]
puts "MASK_TOKENS: $masks"

set drc [make_result_file mask_solve_ok.maskdrc]
set viols [drt::check_mask_drc -output_file $drc]
puts "VIOLATIONS: $viols"

if { $uncolorable != 0 } {
  error "FAIL: 2-colorable layout reported $uncolorable uncolorable"
}
if { $masks == 0 } {
  error "FAIL: solver emitted no coloring for a colorable layout"
}
if { $viols != 0 } {
  error "FAIL: solved coloring is not legal ($viols mask violations)"
}
puts "pass"

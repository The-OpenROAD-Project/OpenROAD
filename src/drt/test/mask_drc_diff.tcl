source "helpers.tcl"
# Different-mask relaxed spacing in the mask-aware DRC audit.
#
# Fixture (mask_drc.def) has, on met1 (MASK 2, min spacing 0.14um = 140 DBU,
# width 0.14um = 140 DBU), four horizontal wires:
#   na (MASK 2) @ y=2000  and  nb (MASK 2) @ y=2240  -> SAME mask
#   nc (MASK 1) @ y=2000  and  nd (MASK 2) @ y=2240  -> DIFFERENT mask
# Each colored pair is 240 DBU center-to-center => edge gap = 240-140 = 100
# DBU, which is < the 140 DBU same-mask spacing.
#
# - na/nb (same mask, gap 100 < 140): always a violation.
# - nc/nd (different mask, gap 100): legal under the default relaxed
#   different-mask spacing (0), but a violation once the relaxed spacing is
#   set above the gap.
#
# This proves the audit models BOTH same-mask and different-mask spacing.
read_lef sky130hd/sky130hd_multi_patterned.tlef
read_def mask_drc.def

# Enable mask awareness and set a relaxed different-mask spacing of 0.14um.
# The different-mask pair (gap 100 DBU) is below this and must now be flagged
# in addition to the same-mask pair.
set_mask_aware_routing -enable -different_mask_spacing 0.14

set drc_file [make_result_file mask_drc_diff.drc]
set viols [drt::check_mask_drc -output_file $drc_file]
puts "VIOLATIONS: $viols"

# Expect exactly two: one same-mask (na/nb) and one different-mask (nc/nd).
if { $viols != 2 } {
  error "expected 2 violations with relaxed diff-mask spacing, got $viols"
}

# Verify the report contains both a same-mask and a different-mask record.
set fh [open $drc_file r]
set data [read $fh]
close $fh
# Same-mask record reports a single mask token: "mask: 2".
if { ![regexp {mask: 2\n} $data] } {
  error "missing same-mask (na/nb) violation record"
}
# Different-mask record reports both mask tokens: "mask: 1 2" or "mask: 2 1".
if { ![regexp {mask: (1 2|2 1)\n} $data] } {
  error "missing different-mask (nc/nd) violation record"
}
puts "DIFF_MASK_OK: both same- and different-mask violations flagged"

# Sanity: with the default relaxed spacing (0), only the same-mask pair is a
# violation -- different-mask pairs are legal at the same distance.
set_mask_aware_routing -enable -different_mask_spacing 0
set drc_file0 [make_result_file mask_drc_diff0.drc]
set viols0 [drt::check_mask_drc -output_file $drc_file0]
puts "VIOLATIONS_DEFAULT: $viols0"
if { $viols0 != 1 } {
  error "expected 1 violation with default diff-mask spacing, got $viols0"
}
puts "DEFAULT_OK: different-mask pair legal at same distance"

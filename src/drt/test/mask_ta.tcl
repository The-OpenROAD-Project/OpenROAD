# Slice 4: mask-aware TRACK ASSIGNMENT.
#
# With mask-aware mode ON, the track-assignment (TA) stage actively biases its
# track choice by mask color (track parity) so that same-mask neighbors respect
# the same-mask spacing. Route a real multi-patterned design (met1..met5 are
# double-patterned, MASK 2) with mask-aware ON, then verify the resulting
# coloring is LEGAL via check_mask_drc (slice 1): 0 same-mask spacing
# violations.
#
# The flag-OFF path is proven byte-identical to baseline separately;
# with the flag off the TA mask-cost term is identically 0.
source "helpers.tcl"
read_lef "sky130hd/sky130hd_multi_patterned.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "gcd_sky130hd.def"
read_guides "gcd_sky130hd.guide"

set_mask_aware_routing -enable
set_thread_count 1
set_routing_layers -signal met1-met5
detailed_route -verbose 0

set on_def [make_result_file mask_ta_on.def]
write_def $on_def

# Sanity: multi-mask-layer segments carry assigned colors in the output.
proc count_mask_tokens { def_path } {
  set fh [open $def_path r]
  set data [read $fh]
  close $fh
  return [regexp -all {\)MASK } $data]
}
set on_masks [count_mask_tokens $on_def]
if { $on_masks == 0 } {
  error "mask-aware TA produced no colored segments"
}
puts "TA_HAS_COLOR: ok"

# The coloring TA chose must be legal: no same-mask spacing violations.
set drc_file [make_result_file mask_ta.maskdrc]
set viols [drt::check_mask_drc -output_file $drc_file]
puts "MASK_VIOLATIONS: $viols"

if { $on_masks > 0 && $viols == 0 } {
  puts "pass"
} else {
  puts "FAIL: on_masks=$on_masks viols=$viols"
}

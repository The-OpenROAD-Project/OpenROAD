# Count routed segments carrying a MASK color token in a DEF file.
proc count_mask_tokens { def_path } {
  set fh [open $def_path r]
  set data [read $fh]
  close $fh
  return [regexp -all {\)MASK } $data]
}

source "helpers.tcl"
# Slice 2: mask-aware routing assigns mask colors to routed shapes.
# Multi-patterned tech: met1/met2/met3 are double-patterned (MASK 2).
# Route a real design with mask-aware mode ON, then verify the router's
# assigned coloring is legal via check_mask_drc (slice 1). This closes the
# route -> colored -> audited loop.
#
# (The flag-OFF path is proven byte-identical to baseline separately; with
# the flag off no coloring code runs at all. The mask_drc.tcl test covers the
# default-OFF gate refusal.)
read_lef "sky130hd/sky130hd_multi_patterned.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "gcd_sky130hd.def"
read_guides "gcd_sky130hd.guide"

set_mask_aware_routing -enable
set_thread_count 1
set_routing_layers -signal met1-met5
detailed_route -verbose 0

set on_def [make_result_file mask_route_on.def]
write_def $on_def

# The router must have assigned mask colors to multi-mask-layer segments.
set on_masks [count_mask_tokens $on_def]
if { $on_masks == 0 } {
  error "mask-aware ON produced no colored segments"
}
puts "ON_HAS_COLOR: ok"

# The assigned coloring must be legal: no same-mask spacing violations.
set drc_file [make_result_file mask_route.maskdrc]
set viols [drt::check_mask_drc -output_file $drc_file]
puts "MASK_VIOLATIONS: $viols"

if { $on_masks > 0 && $viols == 0 } {
  puts "pass"
} else {
  puts "FAIL: on_masks=$on_masks viols=$viols"
}

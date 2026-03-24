# Reproduce https://github.com/The-OpenROAD-Project/OpenROAD/issues/4211
#
# A narrow metal2 corridor next to a macro OBS has one track (x=1045)
# where a wire fits (gap=110nm >= 70nm) but the via1 enclosure on
# metal2 (190nm wide) violates the width-dependent spacing rule
# (gap=50nm < 140nm required).
#
# GRT does not consider via spacing when computing corridor capacity,
# so it assigns nets to tracks that have no legal via.  DRT then
# produces unresolvable DRC violations.

source "helpers.tcl"
read_lef "via_spacing_near_macro.lef"
read_def "via_spacing_near_macro.def"

# The guide restricts DRT to the single metal2 track (x=1045) in
# the corridor -- what GRT produces in a congested design.
read_guides "via_spacing_near_macro.guide"

set drc_file [make_result_file via_spacing_near_macro.drc]
detailed_route -output_drc $drc_file -droute_end_iter 0 -verbose 0

# The via at x=1045 inside the corridor produces a Metal Spacing
# violation to the macro OBS.  This is unresolvable.
set f [open $drc_file r]
set drc_content [read $f]
close $f
puts "=== DRC Report ==="
puts $drc_content

if { ![string match "*Metal Spacing*" $drc_content] } {
  error "FAIL: expected Metal Spacing violation from via near macro OBS"
}
puts "PASS: Metal Spacing violation reproduced (issue #4211)"

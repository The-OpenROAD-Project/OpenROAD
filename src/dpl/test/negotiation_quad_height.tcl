# Exercise the negotiation legalizer with a 4-row tall cell (MOCK_QUAD).
# Overlapping multi-height cells (quad, triple, double, single) on the same
# rows force the negotiation loop to resolve conflicts across cells whose
# heights span up to four standard rows.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def negotiation_quad_height.def
set_debug_level DPL negtotiation 1
dpl::detailed_placement_debug -deep_iterative -paint_negotiation_pixels
detailed_placement
check_placement

set def_file [make_result_file negotiation_quad_height.def]
write_def $def_file
# diff_file negotiation_quad_height.defok $def_file

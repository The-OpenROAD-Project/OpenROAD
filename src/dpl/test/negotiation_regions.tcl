# Region-constrained design legalized with the negotiation legalizer.
# Exercises the fence-region path (respectsFence) and region pixel setup
# (groupInitPixels) through the negotiation flow.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def regions1.def
detailed_placement -use_negotiation
check_placement

set def_file [make_result_file negotiation_regions.def]
write_def $def_file
diff_file negotiation_regions.defok $def_file

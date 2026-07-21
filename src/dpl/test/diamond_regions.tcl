# Region-constrained design legalized with the diamond legalizer.
# Exercises the fence-region path (respectsFence) and region pixel setup
# (groupInitPixels) through the diamond search.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def regions1.def
detailed_placement -use_diamond_legalizer
check_placement

set def_file [make_result_file diamond_regions.def]
write_def $def_file
diff_file diamond_regions.defok $def_file

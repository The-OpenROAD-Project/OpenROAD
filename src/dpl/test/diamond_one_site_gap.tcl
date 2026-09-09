# One-site-gap design legalized with the diamond legalizer.
# Exercises the PlacementDRC one-site-gap path through the diamond search.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def one_site_gap_disallow.def
detailed_placement -use_diamond_legalizer
check_placement

set def_file [make_result_file diamond_one_site_gap.def]
write_def $def_file
diff_file diamond_one_site_gap.defok $def_file

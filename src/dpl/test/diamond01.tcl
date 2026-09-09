# gcd global placement legalized with the diamond legalizer.
# Overlapping (global-placed) cells exercise the diamond-search legalization
# path now that negotiation is the default detailed placement algorithm.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_replace.def
detailed_placement -use_diamond_legalizer
check_placement

set def_file [make_result_file diamond01.def]
write_def $def_file
diff_file diamond01.defok $def_file

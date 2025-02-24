source "helpers.tcl"
read_lef Nangate45_data/Nangate45.lef
read_def gcd_no_one_site_gaps.def
improve_placement
check_placement -verbose

set def_file [make_result_file gcd_no_one_site_gaps.def]
write_def $def_file
diff_file gcd_no_one_site_gaps.defok $def_file
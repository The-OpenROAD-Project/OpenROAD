source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def one_site_gap_disallow.def
detailed_placement -disallow_one_site_gaps
check_placement

set def_file [make_result_file one_site_gap_disallow.def]
write_def $def_file
diff_file $def_file one_site_gap_disallow.defok

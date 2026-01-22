source "helpers.tcl"
read_lef Nangate45_data/Nangate45-opt.lef
read_def gcd_no_one_site_sanity.def
# Call DPL multiple times, ensuring no unexpected move occurs.
# Include one_site_gap checks due to previous fixed errors.
# Input def include a one_site_gap with _525_ cell.
detailed_placement
detailed_placement
detailed_placement
detailed_placement
detailed_placement
detailed_placement
check_placement -verbose

set def_file [make_result_file gcd_no_one_site_sanity.def]
write_def $def_file
diff_file gcd_no_one_site_sanity.defok $def_file

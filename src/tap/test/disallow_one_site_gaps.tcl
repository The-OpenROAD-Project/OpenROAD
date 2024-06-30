source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def disallow_one_site_gaps.def

set def_file [make_result_file disallow_one_site_gaps.def]

tapcell -distance "20" -tapcell_master "TAPCELL_X1" \
    -endcap_master "TAPCELL_X1" \
    -disallow_one_site_gaps

write_def $def_file

diff_file disallow_one_site_gaps.defok $def_file

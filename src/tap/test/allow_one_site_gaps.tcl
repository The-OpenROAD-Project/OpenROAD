source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45_data/Nangate45_stdcell_x1.lef
read_def disallow_one_site_gaps.def

set def_file [make_result_file allow_one_site_gaps.def]

tapcell -distance "20" -tapcell_master "TAPCELL_X2" \
    -endcap_master "TAPCELL_X2"

write_def $def_file

diff_file allow_one_site_gaps.defok $def_file

source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_prefix.def

set def_file [make_result_file gcd_prefix.def]

tapcell -distance "20" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1" -tap_prefix "CHECK_TAPCELL_" -endcap_prefix "CHECK_END_"

write_def $def_file

diff_file gcd_prefix.defok $def_file

source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45.def

set def_file [make_result_file gcd_nangate45.def]

tapcell -endcap_cpp "2" -distance "20" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

write_def $def_file

diff_file gcd_nangate45.defok $def_file

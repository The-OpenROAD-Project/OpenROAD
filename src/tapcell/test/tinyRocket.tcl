source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_1024x32.lef
read_lef Nangate45/fakeram45_64x32.lef
read_def tinyRocket.def

set def_file [make_result_file tinyRocket.def]

tapcell -endcap_cpp "2" -distance "120" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

write_def $def_file

diff_file tinyRocket.defok $def_file

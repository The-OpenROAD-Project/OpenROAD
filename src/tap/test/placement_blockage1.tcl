source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def placement_blockage1.def

set def_file [make_result_file placement_blockage1.def]

tapcell -distance "20" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

write_def $def_file

diff_file placement_blockage1.defok $def_file

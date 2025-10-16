source helpers.tcl
read_lef Nangate45/Nangate45_tech.lef
read_lef symmetry.lef
read_def symmetry.def

set def_file [make_result_file symmetry.def]

tapcell -distance 120 -tapcell_master "TAPCELL" -endcap_master "TAPCELL"

write_def $def_file

diff_file symmetry.defok $def_file

source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def gcd_ripup.def

tapcell -distance "20" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

set def_file1 [make_result_file mc1.def]
write_def $def_file1

tapcell -distance "10" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

set def_file2 [make_result_file mc2.def]
write_def $def_file2

diff_file $def_file1 $def_file2

source "helpers.tcl"

read_lef NangateOpenCellLibrary.invert_viarule.lef
read_def gcd/floorplan.def

pdngen gcd/PDN.cfg -verbose

set def_file results/test_gcd.invert_viarule.def
write_def $def_file 

diff_files $def_file test_gcd.invert_viarule.defok

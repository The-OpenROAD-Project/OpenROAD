source "helpers.tcl"

read_lef nangate45/NangateOpenCellLibrary.mod.lef
read_def gcd/floorplan.def

pdngen gcd/PDN.cfg -verbose

set def_file results/test_gcd.def
write_def $def_file 

diff_files $def_file test_gcd.defok

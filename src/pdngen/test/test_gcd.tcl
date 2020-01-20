source "helpers.tcl"
read_lef nangate45/NangateOpenCellLibrary.mod.lef
read_def gcd/floorplan.def
pdngen gcd/PDN.cfg -verbose
set def_file results/test_gcd.def
#write_def $def_file
set db [::ord::get_db]
set block [[$db getChip] getBlock]
odb::odb_write_def $block $def_file DEF_5_6

diff_files $def_file test_gcd.defok

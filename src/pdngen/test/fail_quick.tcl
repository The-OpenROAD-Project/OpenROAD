source "helpers.tcl"

read_lef nangate45/NangateOpenCellLibrary.mod.lef
read_def fail_quick/floorplan.def

pdngen fail_quick/PDN.cfg -verbose

set def_file results/fail_quick.def
write_def $def_file 

diff_files $def_file fail_quick.defok

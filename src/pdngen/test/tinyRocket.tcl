source "helpers.tcl"

read_lef nangate45/NangateOpenCellLibrary.mod.lef
read_lef tinyRocket/fakeram45_64x32.lef
read_lef tinyRocket/fakeram45_1024x32.lef

read_def tinyRocket/2_5_floorplan_tapcell.def

pdngen tinyRocket/PDN.cfg -verbose

set def_file results/tinyRocket.def
write_def $def_file 

diff_files $def_file tinyRocket.defok

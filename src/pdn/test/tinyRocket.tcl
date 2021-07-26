source "helpers.tcl"
source ../src/PdnGen.tcl

read_lef ../../../test/Nangate45/Nangate45.lef
read_lef fakeram45_64x32.lef
read_lef fakeram45_1024x32.lef

read_def tinyRocket/2_5_floorplan_tapcell.def

pdngen tinyRocket/PDN.cfg -verbose

set def_file results/tinyRocket.def
write_def $def_file 

diff_files $def_file tinyRocket.defok

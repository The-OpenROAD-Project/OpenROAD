read_lef nangate45/NangateOpenCellLibrary.mod.lef
read_def gcd/floorplan.def
pdngen gcd/PDN.cfg
set def_file results/test_gcd.def
write_def $def_file

set f [open $def_file]
fcopy $f stdout
close $f

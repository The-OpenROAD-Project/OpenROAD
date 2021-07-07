source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_def gcd_dual_rails/floorplan.def

pdngen gcd_dual_rails/PDN.cfg -verbose

set def_file results/test_gcd_dual_rails.def
write_def $def_file 

diff_files $def_file test_gcd_dual_rails.defok

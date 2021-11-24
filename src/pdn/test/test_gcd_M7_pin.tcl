source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_def gcd_M7_pin/floorplan.def

pdngen gcd_M7_pin/PDN.cfg -verbose

set def_file results/test_gcd_M7_pin.def
write_def $def_file 

diff_files $def_file test_gcd_M7_pin.defok

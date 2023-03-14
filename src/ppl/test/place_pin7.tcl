# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def gcd_sky130.def

place_pin -pin_name clk \
    -layer met2 \
    -location "8 0" \
    -force_to_die_boundary

set def_file [make_result_file place_pin7.def]

write_def $def_file

diff_file place_pin7.defok $def_file

source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef

read_def ta_pin_aligned.def
read_guides ta_pin_aligned.route_guide

detailed_route -verbose 0

set def_file [make_result_file ta_pin_aligned.def]
write_def $def_file

diff_files ta_pin_aligned.defok $def_file

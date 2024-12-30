source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lib Nangate45/Nangate45_typ.lib
read_def "partial_tracks.def"

set def_file [make_result_file partial_tracks.def]

place_pin -pin_name "clk" -layer metal2 -location "4 4" -force_to_die_boundary

write_def $def_file

diff_file partial_tracks.defok $def_file

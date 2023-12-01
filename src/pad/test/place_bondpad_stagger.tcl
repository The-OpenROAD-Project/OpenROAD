# Test for placing pads with staggering
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads_stagger.lef

read_def Nangate45_blackparrot/floorplan_stagger.def

place_bondpad -bond PAD -offset "-5 75" u_*_inner
place_bondpad -bond PAD -offset "-5 -5" u_*_outer

set def_file [make_result_file "place_bondpad_stagger.def"]
write_def $def_file
diff_files $def_file "place_bondpad_stagger.defok"

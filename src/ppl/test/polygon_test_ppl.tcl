source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def polygon_test_ppl.def

make_tracks
place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 -min_distance 0.12 -is_rectilinear_die

set def_file [make_result_file polygon_test_ppl.def]
write_def $def_file
# diff_files polygon_test_ppl.defok $def_file

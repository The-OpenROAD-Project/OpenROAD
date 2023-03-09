source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def mock.def
initialize_floorplan -utilization 30 -aspect_ratio 0.5

set def_file [make_result_file mock.def]
write_def $def_file
diff_files mock.defok $def_file
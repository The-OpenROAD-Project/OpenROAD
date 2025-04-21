source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def multi_height1-opt.def
improve_placement
check_placement

set def_file [make_result_file multi_height1-opt.def]
write_def $def_file
diff_file $def_file multi_height1-opt.defok

source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_lef edge_spacing.lef
read_def edge_spacing-opt.def
improve_placement
check_placement

set def_file [make_result_file edge_spacing-opt.def]
write_def $def_file
diff_file $def_file edge_spacing-opt.defok

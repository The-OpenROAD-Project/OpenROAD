source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef no_site_block.lef
read_def no_site_block-opt.def
improve_placement
check_placement

set def_file [make_result_file no_site_block-opt.def]
write_def $def_file
diff_file $def_file no_site_block-opt.defok

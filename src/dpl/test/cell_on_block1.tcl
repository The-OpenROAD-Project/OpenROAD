# std cells on top of block
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_lef block2.lef
read_def cell_on_block1.def
detailed_placement

set def_file [make_result_file cell_on_block1.def]
write_def $def_file
diff_file cell_on_block1.defok $def_file

# std cells on top of block with site-less channels leading to hopeless pixels
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_lef cell_on_block2.lef
read_def cell_on_block2.def
set_placement_padding -global -right 4
detailed_placement

set def_file [make_result_file cell_on_block2.def]
write_def $def_file
diff_file cell_on_block2.defok $def_file

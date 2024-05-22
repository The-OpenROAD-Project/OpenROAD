source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def blockage1.def
improve_placement -max_displacement {5 1}
check_placement

set def_file [make_result_file blockage1.def]
write_def $def_file
diff_file $def_file blockage1.defok

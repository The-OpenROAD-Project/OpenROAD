# filler_placement with a cell that has no implant layer
source "helpers.tcl"
read_lef fillers12.lef
read_def fillers12.def
filler_placement FILLCELL_RVT_X1
check_placement

set def_file [make_result_file fillers12.def]
write_def $def_file
diff_file $def_file fillers12.defok

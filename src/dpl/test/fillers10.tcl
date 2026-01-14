# filler_placement for simple09 with block included as filler
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def simple09.def
detailed_placement
filler_placement -verbose "fake* FILL*"
check_placement

set def_file [make_result_file fillers10.def]
write_def $def_file
diff_file $def_file fillers10.defok

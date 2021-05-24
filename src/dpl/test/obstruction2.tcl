# ram with obstruction
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def obstruction2.def
detailed_placement
filler_placement FILLCELL*
check_placement

set def_file [make_result_file obstruction2.def]
write_def $def_file
diff_file obstruction2.defok $def_file

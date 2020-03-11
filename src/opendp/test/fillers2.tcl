# place_fillers with set_placement_padding
source helpers.tcl
read_lef Nangate45.lef
read_def simple01.def
set_placement_padding -global -left 2 -right 2
detailed_placement
filler_placement FILL*
check_placement

set def_file [make_result_file fillers2.def]
write_def $def_file
diff_file $def_file fillers2.defok

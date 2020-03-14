# off grid inst overlapping row site
source helpers.tcl
read_lef Nangate45.lef
read_def overlap1.def
detailed_placement
check_placement -verbose

set def_file [make_result_file overlap1.def]
write_def $def_file
diff_file overlap1.defok $def_file

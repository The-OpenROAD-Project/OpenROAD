# no fillers in cut rows
source helpers.tcl
read_lef Nangate45.lef
read_def cut_rows1.def
detailed_placement
check_placement
filler_placement FILL*
check_placement

set def_file [make_result_file fillers5.def]
write_def $def_file
diff_file fillers5.defok $def_file

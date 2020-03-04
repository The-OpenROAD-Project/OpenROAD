# place_fillers for simple01
source helpers.tcl
read_lef Nangate45.lef
read_def simple01.def
legalize_placement
place_fillers FILL*

set def_file [make_result_file fillers1.def]
write_def $def_file
diff_file $def_file fillers1.defok

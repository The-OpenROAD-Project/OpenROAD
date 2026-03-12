# filler_placement for simple09 with a floating pin
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def simple09.def

# Create a floating pin
set net [odb::dbNet_create [ord::get_db_block] "float"]
odb::dbBTerm_create $net "float"

detailed_placement
filler_placement -verbose "fake* FILL*"
check_placement

set def_file [make_result_file fillers11.def]
write_def $def_file
diff_file $def_file fillers11.defok

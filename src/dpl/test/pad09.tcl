# corner pad/endcaps with obstructions
source -echo "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_lef pad09.lef
read_def pad09.def

detailed_placement
filler_placement FILLCELL*

set def_file [make_result_file pad09.def]
write_def $def_file
diff_file pad09.defok $def_file

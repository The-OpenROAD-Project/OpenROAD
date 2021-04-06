# macro with obstruction
source -echo "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_lef obstruction1.lef
read_def obstruction1.def

detailed_placement

set def_file [make_result_file obstruction1.def]
write_def $def_file
diff_file obstruction1.defok $def_file

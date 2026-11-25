# Repeated supply-via array: exercise finite extent, an intentional hole, and
# repeated placement in both row orientations.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef pdn_via_array.lef
read_def pdn_via_array.def

detailed_placement -pdn_aware
check_placement

set def_file [make_result_file pdn_via_array.def]
write_def $def_file
check "placement DEF" {diff_files pdn_via_array.defok $def_file} 0

exit_summary

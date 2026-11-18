# Check strict Euclidean routing spacing, diagonal equality, the core boundary,
# and the same-net exemption for fixed supply via shapes.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef pdn_via_euclidean.lef
read_def pdn_via_euclidean.def

detailed_placement -pdn_aware
check_placement

set def_file [make_result_file pdn_via_euclidean.def]
write_def $def_file
check "placement DEF" {diff_files pdn_via_euclidean.defok $def_file} 0

exit_summary

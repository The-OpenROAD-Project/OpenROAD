# Fixed supply via CUT constituents use Euclidean spacing.  The routing
# constituents are intentionally tiny, so these cases isolate cut spacing
# rather than a metal keepout.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef pdn_via_cut.lef
read_def pdn_via_cut.def

detailed_placement -pdn_aware
check_placement

set def_file [make_result_file pdn_via_cut.def]
write_def $def_file
check "placement DEF" {diff_files pdn_via_cut.defok $def_file} 0

exit_summary

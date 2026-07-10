# Mixed-cell-height design (1x/2x/3x rows) legalized with the negotiation
# legalizer. Overlapping, off-die single/double/triple-height cells exercise
# the multi-row placement path (isValidRow, row grouping) through negotiation.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_def multi_height_rows.def
detailed_placement -use_negotiation
check_placement

set def_file [make_result_file negotiation_multi_height.def]
write_def $def_file
diff_file negotiation_multi_height.defok $def_file

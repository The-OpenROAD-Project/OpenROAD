source "helpers.tcl"

# example.3dbx instantiates one chiplet master (SoC) twice (soc_inst,
# soc_inst_duplicate). Duplicated masters are not supported for timing in
# this version: descending a block placed more than once would alias its
# inner dbInsts across placements. read_3dbx must SUCCEED (the structural
# 3DBlox model is valid odb data), while STA warns (STA-3004) and declines
# to create the 3DIC timing network (STA-3000).
# (Real duplicated-master support needs per-unfold-path instance identity.)
read_3dbx ../../odb/test/data/example.3dbx

# The structural model is intact and readable after the declined network.
set db [ord::get_db]
set top_chip [$db getChip]
check "top chip name" { $top_chip getName } TopDesign
check "chip inst count" { llength [$top_chip getChipInsts] } 2

exit_summary

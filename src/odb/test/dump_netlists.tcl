source "helpers.tcl"

set db [ord::get_db]
read_lef "../../../test/Nangate45/Nangate45.lef"
read_def "data/gcd/gcd.def"
set chip [$db getChip]
set block [$chip getBlock]

write_cdl -masters NangateOpenCellLibrary.cdl "[make_result_file gcd.cdl]"

diff_files "[make_result_file gcd.cdl]" dump_netlists_cdl.ok

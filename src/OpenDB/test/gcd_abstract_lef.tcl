source "helpers.tcl"

# Open database, load lef and design

set db [ord::get_db]
read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
read_def "data/gcd/floorplan.def"
set chip [$db getChip]

set lef_file [make_result_file gcd_abstract_lef.lef]

set block [$chip getBlock]


$block saveLef $lef_file

exit_summary

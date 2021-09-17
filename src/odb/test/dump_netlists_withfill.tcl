source "helpers.tcl"


set db [ord::get_db]
read_lef "../../../test/Nangate45/Nangate45.lef"
read_def "data/gcd/gcd.def"
set chip [$db getChip]
set block [$chip getBlock]

write_cdl -include_fillers -masters NangateOpenCellLibrary.cdl "results/gcdwithfill.cdl"

set isDiff [diff_files "results/gcdwithfill.cdl" "dump_netlists_withfill_cdl.ok"]


if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit

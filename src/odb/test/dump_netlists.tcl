source "helpers.tcl"

set db [ord::get_db]
read_lef "../../../test/Nangate45/Nangate45.lef"
read_def "data/gcd/gcd.def"
set chip [$db getChip]
set block [$chip getBlock]

write_cdl -masters NangateOpenCellLibrary.cdl "results/gcd.cdl"

set isDiff [diff_files "results/gcd.cdl" "dump_netlists_cdl.ok"]

if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit

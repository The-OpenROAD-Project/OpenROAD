source "helpers.tcl"


set db [odb::dbDatabase_create]
odb::read_lef $db "data/Nangate45/Nangate45.lef"
set chip [odb::read_def $db "data/gcd/gcd.def"]
set block [$chip getBlock]

odb::dump_netlist $block "results/gcdwithfill.cdl" 1

set isDiff [diff_files "results/gcdwithfill.cdl" "dump_netlists_withfill_cdl.ok"]


if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit

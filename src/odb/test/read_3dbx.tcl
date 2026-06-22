source "helpers.tcl"

set db [ord::get_db]

read_3dbx "data/example.3dbx"
if {[$db getChip] == "NULL"} {
    puts "FAIL: Read 3dbx Failed"
    exit 1
}
check "Unfolded Chip Insts Count " { llength [$db getUnfoldedChipInsts] } 2
check "Unfolded Chip Conns Count " { llength [$db getUnfoldedChipConns] } 2
check "Unfolded Chip Regions Count " { llength [$db getUnfoldedChipRegionInsts] } 4
check "Unfolded Chip Bumps Count " { llength [$db getUnfoldedChipBumpInsts] } 4
check "Unfolded Chip Bumps Count " { llength [$db getUnfoldedChipNets] } 0

puts "pass"

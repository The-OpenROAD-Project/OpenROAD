source "helpers.tcl"

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db "data/gscl45nm.lef"]
odb::read_def $db "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]
set lef_write_result [odb::write_lef $lib "results/test.lef"]
if {$lef_write_result != 1} {
    puts "FAIL: lef write error"
    exit 1
}
set def_write_result [odb::write_def $block "results/test.def"]
if {$def_write_result != 1} {
    puts "FAIL: def write error"
    exit 1
}

puts "pass"
exit 0

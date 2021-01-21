source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
set lib [$db findLib gscl45nm]
read_def "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]

set lef_write_result [odb::write_lef $lib "results/test.lef"]
if {$lef_write_result != 1} {
    puts "FAIL: lef write error"
    exit 1
}
write_def "results/test.def"
puts "pass"
exit 0

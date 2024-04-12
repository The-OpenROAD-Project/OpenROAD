source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
set lib [$db findLib gscl45nm]
read_def "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]

set out_lef [make_result_file "write_lef_and_def.lef"]
set lef_write_result [odb::write_lef $lib $out_lef]
if {$lef_write_result != 1} {
    puts "FAIL: lef write error"
    exit 1
}

diff_files $out_lef "write_lef_and_def.lefok"

set out_def [make_result_file "write_lef_and_def.def"]
write_def $out_def

diff_files $out_def "write_lef_and_def.defok"

puts "pass"
exit 0

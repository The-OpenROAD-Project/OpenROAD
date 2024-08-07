source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm_polygon.lef"
set lib [$db findLib gscl45nm_polygon]
read_def "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]

set out_lef [make_result_file "write_lef_polygon.lef"]
set lef_write_result [odb::write_lef $lib $out_lef]
if {$lef_write_result != 1} {
    puts "FAIL: lef write error"
    exit 1
}

diff_files $out_lef "write_lef_polygon.lefok"

puts "pass"
exit 0

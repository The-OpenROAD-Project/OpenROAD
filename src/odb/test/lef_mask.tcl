source "helpers.tcl"

set db [ord::get_db]
read_lef "data/mask.lef"

set lef_file [make_result_file lef_mask.lef]
set lef_write_result [odb::write_tech_lef [$db getTech] $lef_file]
if {$lef_write_result != 1} {
    puts "FAIL: lef write error"
    exit 1
}
diff_files lef_mask.lefok $lef_file
puts "pass"
exit 0

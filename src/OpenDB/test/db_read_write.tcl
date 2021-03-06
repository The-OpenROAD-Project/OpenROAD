source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design.def"

set db_file "results/export.db"
set write_result [odb::write_db $db $db_file]
if {!$write_result} {
    puts "FAIL: Write DB failed"
    exit 1
}

set new_db [odb::dbDatabase_create]
odb::read_db $new_db $db_file

if { [odb::db_diff $db $new_db] } {
  puts "FAIL: Differences found between exported and imported db"
  exit 1
}

puts "pass"
exit 0

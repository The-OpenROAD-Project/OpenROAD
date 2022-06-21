# This test ensures the ODB version is updated when the database changes.
# Upon failure, update the ODB version in odb/src/db/dbDatabase.h and run update_db.tcl
source "helpers.tcl"

set db [ord::get_db]
read_db "data/design.odb"
if {[$db getChip] == "NULL"} {
    puts "FAIL: Read ODB Failed, update ODB version and run update_db.tcl"
    exit 1
}

puts "pass"
exit 0

source "helpers.tcl"

set db [odb::dbDatabase_create]
if {$db == "NULL"} {
    puts "FAIL: Create DB Failed"    
}
puts "pass"    
exit 0

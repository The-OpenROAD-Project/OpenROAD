source "helpers.tcl"

set db [ord::get_db]

read_3dbx "data/example.3dbx"
if { [$db getChip] == "NULL" } {
  puts "FAIL: Read 3dbx Failed"
  exit 1
}

puts "pass"

source "helpers.tcl"

set db [ord::get_db]

read_3dbv "data/example.3dbv"
if { [$db getChip] == "NULL" } {
  puts "FAIL: Read 3dbv Failed"
  exit 1
}

puts "pass"

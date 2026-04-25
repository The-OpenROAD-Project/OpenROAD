source "helpers.tcl"

set db [ord::get_db]

read_3dbx "data/example.3dbx"
if { [$db getChip] == "NULL" } {
  puts "FAIL: read_3dbx did not create a chip"
  exit 1
}
if { [llength [$db getChips]] <= 1 } {
  puts "FAIL: expected chiplet defs alongside the top chip"
  exit 1
}

clear_3dbx
if { [$db getChip] != "NULL" } {
  puts "FAIL: clear_3dbx did not remove the top chip"
  exit 1
}
if { [llength [$db getChips]] != 0 } {
  puts "FAIL: clear_3dbx left chiplet defs behind: [$db getChips]"
  exit 1
}

# Must be idempotent when nothing is loaded.
clear_3dbx

# Second read with the same chiplet names must succeed — this is the
# user-visible regression clear_3dbx needs to prevent: without destroying
# the chiplet defs, this errors with ODB-0530 "Chiplet X already exists".
read_3dbx "data/example.3dbx"
if { [$db getChip] == "NULL" } {
  puts "FAIL: second read_3dbx after clear_3dbx did not create a chip"
  exit 1
}

puts "pass"

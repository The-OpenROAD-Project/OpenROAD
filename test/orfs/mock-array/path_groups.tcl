source $::env(LOAD_MOCK_ARRAY_TCL)

# Various checks on path groups
set first_path [lindex [find_timing_paths] 0]
set from [get_property [get_property $first_path startpoint] full_name]
set to [get_property [get_property $first_path endpoint] full_name]
puts "Checking if we can find timing paths from $from to $to"
if { [llength [find_timing_paths -from $from -to $to]] == 0 } {
  puts "Error: find_timing_paths -from $from -to $to failed"
  exit 1
}
puts "Checking if we can create a group_path from $from to $to"

# There is no guarantee that the first node in the
# tcl path structure is an exception start point, so use -through
# to ensure we capture a valid path for our testing purposes.
group_path -name test_group -through $from -to $to
if { [llength [find_timing_paths -path_group test_group]] == 0 } {
  puts "Error: find_timing_paths -path_group test_group failed"
  exit 1
}

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

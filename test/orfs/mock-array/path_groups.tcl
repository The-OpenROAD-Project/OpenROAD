source $::env(LOAD_POWER_TCL)

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
group_path -name test_group -from $from -to $to
if { [llength [find_timing_paths -path_group test_group]] == 0 } {
  puts "Error: find_timing_paths -path_group test_group failed"
  exit 1
}

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

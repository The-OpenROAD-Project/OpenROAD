source $::env(LOAD_POWER_TCL)

set instances [get_cells ces*/Multiplier*]
if { [llength $instances] != 64 } {
  puts "Error: Expected to find 64 Multiplier instances, found [llength $instances]"
  exit 1
}

report_power -instances $instances

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

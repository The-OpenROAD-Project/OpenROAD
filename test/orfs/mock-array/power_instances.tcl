source $::env(LOAD_POWER_TCL)

set instances [get_cells *final_adder]
if { [llength $instances] != 4 } {
  puts "Error: Expected to find 4 final_adder instances, found [llength $instances]"
  exit 1
}

report_power -instances $instances

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

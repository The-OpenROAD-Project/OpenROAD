source $::env(LOAD_POWER_TCL)

set instances [get_cells ces*/io_outs_*_mult]
if { [llength $instances] != (64 * 4) } {
  puts "Error: Expected to find 64 Elements * 4 Multiplier instances, found [llength $instances]"
  exit 1
}

report_power -instances $instances

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

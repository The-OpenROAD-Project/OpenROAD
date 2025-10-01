source $::env(LOAD_POWER_TCL)

set instances [get_cells ces*/io_outs_*_mult]

set num [expr $::env(ARRAY_COLS) * $::env(ARRAY_ROWS)]

if { [llength $instances] != $num } {
  puts "Error: Expected to find $num instances, found [llength $instances]"
  exit 1
}

report_power -instances $instances

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

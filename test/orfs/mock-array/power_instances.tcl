source $::env(LOAD_MOCK_ARRAY_TCL)

set vcd_file $::env(VCD_STIMULI)
log_cmd read_vcd -scope TOP/MockArray $vcd_file

set instances [get_cells ces*/io_outs_*_mult]

set num [expr $::env(ARRAY_COLS) * $::env(ARRAY_ROWS) * 4]

if { [llength $instances] != $num } {
  puts "Error: Expected to find $num instances, found [llength $instances]"
  exit 1
}

report_power -instances $instances

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

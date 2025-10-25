source $::env(LOAD_POWER_TCL)

# OpenSTA reports reg2reg paths inside macros,
# whereas these paths are hidden to OpenROAD that
# uses a .lib file for the macros.
log_cmd report_checks

log_cmd report_power

set fp [open $::env(RESULTS_DIR)/activity.tcl w]
set pins [get_pins -hierarchical *]
set clock_period [expr [get_property [get_clocks] period] * 1e-12]
foreach pin $pins {
  set activity [get_property $pin activity]
  set activity_origin [lindex $activity 2]
  if { $activity_origin != "vcd" } {
    continue
  }
  set duty [lindex $activity 1]
  if { $duty > 1.0 } { # this generates an sta error
    set duty 1.0
  }
  puts $fp "set_power_activity \
  -pin \[get_pins \{[get_property $pin full_name]\}\] \
  -activity [expr [lindex $activity 0] * $clock_period] \
  -duty $duty"
}
close $fp

puts "Total number of pins: [llength [get_pins -hierarchical *]]"
set no_vcd_activity {}
foreach pin $pins {
  set activity [get_property $pin activity]
  set activity_origin [lindex $activity 2]
  if { $activity_origin == "vcd" } {
    continue
  }
  if { $activity_origin == "constant" } {
    continue
  }
  if { $activity_origin == "unknown" } {
    continue
  }
  if { [get_property $pin is_hierarchical] } {
    continue
  }
  if { $activity_origin == "clock" } {
    continue
  }
  set direction [get_property $pin direction]
  if { $direction == "internal" } {
    continue
  }
  lappend no_vcd_activity "[get_full_name $pin] $activity $direction"
  if { [llength $no_vcd_activity] >= 10 } {
    break
  }
}

if { [llength $no_vcd_activity] > 0 } {
  puts "Error: Listing [llength $no_vcd_activity] pins without activity from $vcd_file:"
  foreach pin $no_vcd_activity {
    puts $pin
  }
  exit 1
}

set ces {}
for { set x 0 } { $x < 8 } { incr x } {
  for { set y 0 } { $y < 8 } { incr y } {
    lappend ces ces_${x}_${y}
  }
}

puts {report_power -instances [get_cells $ces]}
report_power -instances [get_cells $ces]

proc total_power { } {
  return [lindex [sta::design_power [sta::corners]] 3]
}


set total_power_vcd [total_power]
log_cmd report_power

source $::env(RESULTS_DIR)/activity.tcl
log_cmd report_power
set total_power_user_activity [total_power]

puts "Total power from VCD: $total_power_vcd"
puts "Total power from user activity: $total_power_user_activity"

if { $total_power_vcd == $total_power_user_activity } {
  puts "Error: settting user power activity had no effect, expected some loss in accuracy"
  exit 1
}

if { abs($total_power_vcd - $total_power_user_activity) > 1e-3 } {
  puts "Error: Total power mismatch between VCD and user activity:\
          $total_power_vcd vs $total_power_user_activity"
  exit 1
}

log_cmd report_parasitic_annotation
log_cmd report_activity_annotation -report_unannotated

# some drive-by tests for OpenSTA path groups
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

source $::env(SCRIPTS_DIR)/util.tcl

# --- Helper Procedures ---

proc read_lefs { } {
  if { [string match "*openroad" $::env(OPENROAD_EXE)] } {
    puts "Reading LEF files for OpenROAD"
    log_cmd read_lef $::env(TECH_LEF)
    log_cmd read_lef $::env(SC_LEF)
    if { [env_var_exists_and_non_empty ADDITIONAL_LEFS] } {
      foreach lef $::env(ADDITIONAL_LEFS) {
        log_cmd read_lef $lef
      }
    }
  }
}

proc get_total_power { } {
  return [lindex [sta::design_power [sta::scenes]] 3]
}

# Verify that pins have activity annotated from VCD
proc verify_vcd_annotation { vcd_file pins } {
  set no_vcd_activity {}
  foreach pin $pins {
    set activity [get_property $pin activity]
    set origin [lindex $activity 2]

    # Skip criteria
    if {
      $origin == "vcd" || $origin == "constant" || $origin == "unknown" ||
      $origin == "clock" || [get_property $pin is_hierarchical] ||
      [get_property $pin direction] == "internal"
    } {
      continue
    }

    lappend no_vcd_activity "[get_full_name $pin] $activity"
    if { [llength $no_vcd_activity] >= 10 } {
      break
    }
  }

  if { [llength $no_vcd_activity] > 0 } {
    puts "Error: Found [llength $no_vcd_activity] pins without VCD activity from $vcd_file:"
    foreach item $no_vcd_activity { puts $item }
    exit 1
  }
}

# Generate a Tcl file to set and verify user-defined activity
proc write_user_activity_tcl { filename pins clock_period } {
  set fp [open $filename w]
  puts $fp "set clock_period $clock_period"
  puts $fp {proc read_back_and_compare { pin_name expected_activity expected_duty clock_period } {
    set pin [get_pins $pin_name]
    set activity [get_property $pin activity]
    set rb_activity [expr [lindex $activity 0] * $clock_period]
    set rb_duty [lindex $activity 1]
    if { abs($rb_activity - $expected_activity) > 1e-3 } {
      puts "Error: activity mismatch on $pin_name: expected $expected_activity, got $rb_activity"
      exit 1
    }
    if { abs($rb_duty - $expected_duty) > 1e-3 } {
      puts "Error: duty mismatch on $pin_name: expected $expected_duty, got $rb_duty"
      exit 1
    }
  }}

  set set_cmds {}
  set check_cmds {}
  foreach pin $pins {
    set activity [get_property $pin activity]
    if { [lindex $activity 2] != "vcd" } { continue }

    set duty [lindex $activity 1]
    if { $duty > 1.0 } { set duty 1.0 }

    set activity_val [expr [lindex $activity 0] * $clock_period]
    set pin_name [get_property $pin full_name]

    lappend set_cmds "set_power_activity -pin \[get_pins \{$pin_name\}\] \
      -activity $activity_val -duty $duty"
    lappend check_cmds "read_back_and_compare \{$pin_name\} $activity_val $duty \$clock_period"
  }

  foreach cmd $set_cmds { puts $fp $cmd }
  foreach cmd $check_cmds { puts $fp $cmd }
  close $fp
}

# --- Main Flow ---

read_lefs
source $::env(LOAD_MOCK_ARRAY_TCL)

#------------------------------------------------
# Stage 1: Estimated Power (Before reading VCD)
#------------------------------------------------
set pwr_est [get_total_power]
puts "Power (Estimation): $pwr_est"
log_cmd report_power

#------------------------------------------------
# Stage 2: VCD-based Power
#------------------------------------------------
set vcd_file $::env(VCD_STIMULI)
log_cmd read_vcd -scope TOP/MockArray $vcd_file
set pwr_vcd [get_total_power]
puts "Power (VCD): $pwr_vcd"

# Verification: VCD should change the power value
if { $pwr_est == $pwr_vcd } {
  puts "Error: VCD activity had no effect on power calculation (remains $pwr_vcd)"
  exit 1
}

# Standard reports
log_cmd report_checks
log_cmd report_power

# Prepare for User Activity Stage
set pins [get_pins -hierarchical *]
set clock_period [expr [get_property [get_clocks] period] * 1e-12]
set activity_file "$::env(RESULTS_DIR)/activity.tcl"

verify_vcd_annotation $vcd_file $pins
write_user_activity_tcl $activity_file $pins $clock_period

# Report power for array instances
set ces {}
for { set x 0 } { $x < $::env(ARRAY_COLS) } { incr x } {
  for { set y 0 } { $y < $::env(ARRAY_ROWS) } { incr y } {
    lappend ces ces_${x}_${y}
  }
}
puts "report_power -instances \[get_cells \$ces\]"
report_power -instances [get_cells $ces]

#------------------------------------------------
# Stage 3: User Activity Power
#------------------------------------------------
source $activity_file
log_cmd report_power
set pwr_user [get_total_power]

puts "\nPower Comparison Summary:"
puts "  - Power (Estimation):      $pwr_est"
puts "  - Power (VCD):             $pwr_vcd"
puts "  - Power (User Activity):   $pwr_user"

# Verification: User activity power should match VCD power
# - Note that float accumulation error is allowed.
if { abs($pwr_vcd - $pwr_user) > $pwr_vcd * 0.001 } {
  puts "Error: Large error between VCD power and user activity power"
  exit 1
}

log_cmd report_parasitic_annotation
log_cmd report_activity_annotation -report_unannotated

set f [open $::env(OUTPUT) w]
puts $f OK
close $f

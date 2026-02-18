# Helper functions common to multiple regressions.

if { [info exists ::env(TEST_TMPDIR)] } {
  set test_dir $::env(TEST_TMPDIR)
} else {
  set test_dir [file dirname [file normalize [info script]]]
}

if { [info exists ::env(RESULTS_DIR)] } {
  set result_dir $::env(RESULTS_DIR)
} else {
  set result_dir [file join $test_dir "results"]
}

proc make_result_dir { } {
  variable result_dir
  if { ![file exists $result_dir] } {
    file mkdir $result_dir
  }
  return $result_dir
}

proc make_result_test_dir { subdir } {
  variable result_dir
  set full_path [file join $result_dir $subdir]
  if { ![file exists $full_path] } {
    file mkdir $full_path
  }
  return $full_path
}

proc make_result_file { filename } {
  variable result_dir

  make_result_dir

  set root [file rootname $filename]
  set dir [file dirname $filename]
  make_result_test_dir $dir
  set ext [file extension $filename]
  set filename "$root-tcl$ext"
  return [file join $result_dir $filename]
}

# puts [exec cat $file] without forking.
proc report_file { file } {
  set stream [open $file r]

  while { [gets $stream line] >= 0 } {
    puts $line
  }
  close $stream
}

#==============================================================================
# Routines to run equivalence tests when they are enabled.
proc write_verilog_for_eqy { test stage remove_cells } {
  set netlist [make_result_file "${test}_$stage.v"]
  if { [string equal $remove_cells "None"] } {
    write_verilog $netlist
  } else {
    write_verilog -remove_cells $remove_cells $netlist
  }
}

# Argument Description
# lib_dir:         specifies directory with Verilog library files to be read
#                  alongside tested design
# liberty_files:   specifies list of PDK liberty files to be read
# remove_cells:    specifies remove_cells mode for write_verilog_for_eqy call

sta::define_cmd_args "run_equivalence_test" {
                                            [-lib_dir lib_dir]
                                            [-liberty_files liberty_files]
                                            [-remove_cells remove_cells]
                                          }

proc run_equivalence_test { test args } {
  sta::parse_key_args "run_equivalence_test" args \
    keys {-lib_dir -liberty_files -remove_cells} \
    flags {}

  set lib_files ""
  set liberty_files ""
  set remove_cells "None"

  if { [info exists keys(-lib_dir)] } {
    set lib_files [glob $keys(-lib_dir)/*]
  }

  if { [info exists keys(-liberty_files)] } {
    set liberty_files $keys(-liberty_files)
  }

  if { [info exists keys(-remove_cells)] } {
    set remove_cells $keys(-remove_cells)
  }

  write_verilog_for_eqy $test after $remove_cells
  # eqy config file for test
  set test_script [make_result_file "${test}.eqy"]
  # golden verilog (pre repair_timing)
  set before_netlist [make_result_file "${test}_before.v"]
  # netlist post repair_timing
  set after_netlist [make_result_file "${test}_after.v"]
  # output directory for test
  set run_dir [make_result_file "${test}_output"]
  set outfile [open $test_script w]

  set top_cell [current_design]
  # Gold netlist
  puts $outfile "\[gold\]"
  if { $liberty_files != "" } {
    puts $outfile "read_liberty -ignore_miss_func -overwrite $liberty_files"
  }
  puts $outfile "read_verilog -sv $before_netlist $lib_files"
  puts $outfile "prep -top $top_cell -flatten"
  puts $outfile "memory_map\n"

  # Modified netlist
  puts $outfile "\[gate\]"
  if { $liberty_files != "" } {
    puts $outfile "read_liberty -ignore_miss_func -overwrite $liberty_files"
  }
  puts $outfile "read_verilog -sv $after_netlist $lib_files"
  puts $outfile "prep -top $top_cell -flatten"
  puts $outfile "memory_map\n"

  # Recommendation from eqy team on how to speed up a design
  puts $outfile "\[match *]\ngate-nomatch _*_.*"

  # See issue OpenROAD#6545 "Equivalence check failure due to non-unique resizer nets"
  puts $outfile "gate-nomatch net*"

  # Forbid matching on buffer instances or cloned instances to make it less
  # likely EQY will fail to prove equivalence because of its assuming structural
  # similarity between gold and gate netlists. This doesn't remove coverage.
  puts $outfile "gate-nomatch clone*"
  puts $outfile "gate-nomatch place*"
  puts $outfile "gate-nomatch rebuffer*"
  puts $outfile "gate-nomatch wire*"
  puts $outfile "gate-nomatch place*\n\n"

  # See issue YosysHQ/yosys#5486 "Basic abc script affects equivalence check with eqy"
  # Create a single partition
  puts $outfile "\[collect *\]\ngroup *"

  # Equivalence check recipe
  puts $outfile "\[strategy basic]\nuse sat\ndepth 10\n\n"
  close $outfile

  if { [info exists ::env(EQUIVALENCE_CHECK)] } {
    exec rm -rf $run_dir
    catch { exec eqy -d $run_dir $test_script > /dev/null }
    set count 0
    catch {
      set count [exec grep -c "Successfully proved designs equivalent" \
        $run_dir/logfile.txt]
    }
    if { $count == 0 } {
      puts "Repair timing output failed equivalence test"
    } else {
      puts "Repair timing output passed/skipped equivalence test"
    }
  } else {
    puts "Repair timing output passed/skipped equivalence test"
  }
}
#==============================================================================

proc diff_files { file1 file2 { ignore "" } } {
  set stream1 [open $file1 r]
  set stream2 [open $file2 r]

  set skip false
  set line 1
  set found_diff 0
  set line1_length [gets $stream1 line1]
  set line2_length [gets $stream2 line2]
  while { $line1_length >= 0 && $line2_length >= 0 } {
    if { $ignore ne "" } {
      set skip [regexp $ignore $line1 || regexp $ignore $line2]
    }
    if { !$skip && $line1 != $line2 } {
      set found_diff 1
      break
    }
    incr line
    set line1_length [gets $stream1 line1]
    set line2_length [gets $stream2 line2]
  }
  close $stream1
  close $stream2
  if { $found_diff || $line1_length != $line2_length } {
    puts "Differences found at line $line."
    puts "$line1"
    puts "$line2"
    return 1
  } else {
    puts "No differences found."
    return 0
  }
}

proc run_unit_test_and_exit { relative_path } {
  set test_dir [pwd]
  set openroad_dir [file dirname [file dirname [file dirname $test_dir]]]
  set test_path [file join $openroad_dir {*}$relative_path]

  set test_status [catch { exec sh -c "BASE_DIR=$test_dir $test_path" } output option]

  puts $test_status
  puts $output
  if { $test_status != 0 } {
    set test_err_info [lassign [dict get $option -errorcode] err_type]
    switch -exact -- $err_type {
      NONE {
        #passed
      }
      CHILDSTATUS {
        # non-zero exit status
        set exit_status [lindex $test_err_info 1]
        puts "ERROR: test returned exit code $exit_status"
        exit 1
      }
      default {
        puts "ERROR: $option"
        exit 1
      }
    }
  }
  puts "pass"
  exit 0
}

# Note: required e.g. in CentOS 7 environment.
if { [package vcompare [package present Tcl] 8.6] == -1 } {
  # tclint-disable-next-line redefined-builtin
  proc lmap { args } {
    set result {}
    set var [lindex $args 0]
    foreach item [lindex $args 1] {
      uplevel 1 "set $var $item"
      lappend result [uplevel 1 [lindex $args end]]
    }
    return $result
  }
}

set ::failing_checks 0
set ::passing_checks 0

proc check { description test expected_value } {
  if { [catch { set return_value [uplevel 1 $test] } msg] } {
    incr ::failing_checks
    error "FAIL: $description: Command \{$test\}\n$msg"
  } elseif { $return_value != $expected_value } {
    incr ::failing_checks
    error "FAIL: $description: Expected $expected_value, got $return_value"
  } else {
    incr ::passing_checks
  }
}

proc exit_summary { } {
  set total_checks [expr { $::passing_checks + $::failing_checks }]
  if { $total_checks > 0 } {
    set pass_per [expr { round(100.0 * $::passing_checks / $total_checks) }]
    puts "Summary $::passing_checks / $total_checks (${pass_per}% pass)"

    if { $total_checks == $::passing_checks } {
      puts "pass"
    }
  } else {
    puts "Summary 0 checks run"
  }
  exit $::failing_checks
}

# Reading DEF file
suppress_message ODB 127
# Finished DEF file
suppress_message ODB 134

# suppress tap info messages
suppress_message TAP 100
suppress_message TAP 101

# suppress par messages with filenames
suppress_message PAR 6
suppress_message PAR 38

# suppress ord message with number of threads
suppress_message ORD 30

# suppress grt message with the suggested adjustment
suppress_message GRT 303
suppress_message GRT 704

proc get_3dblox_marker_count { category_name } {
  set top_chip [[ord::get_db] getChip]
  if { $top_chip == "NULL" } {
    return 0
  }
  set top_category [$top_chip findMarkerCategory "3DBlox"]
  if { $top_category == "NULL" } {
    return 0
  }

  set category [$top_category findMarkerCategory $category_name]
  if { $category != "NULL" } {
    return [$category getMarkerCount]
  }
  return 0
}

proc get_3dblox_connected_errors { } {
  return [get_3dblox_marker_count "Connected regions"]
}

# Helper functions common to multiple regressions.

set test_dir [file dirname [file normalize [info script]]]
set result_dir [file join $test_dir "results"]

proc make_result_file { filename } {
  variable result_dir
  if { ![file exists $result_dir] } {
    file mkdir $result_dir
  }
  set root [file rootname $filename]
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

proc diff_files { file1 file2 } {
  set stream1 [open $file1 r]
  set stream2 [open $file2 r]
  
  set line 1
  set found_diff 0
  set line1_length [gets $stream1 line1]
  set line2_length [gets $stream2 line2]
  while { $line1_length >= 0 && $line2_length >= 0 } {
    if { $line1 != $line2 } {
      set found_diff 1
      break
    }
    incr line
    set line1_length [gets $stream1 line1]
    set line2_length [gets $stream2 line2]
  }
  close $stream1
  close $stream2
  if { $found_diff || $line1_length != $line2_length} {
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

# Output voltage file is specified as ...
suppress_message PSM 2
# Output current file specified ...
suppress_message PSM 3
# Output spice file is specified as
suppress_message PSM 5
# SPICE file is written at
suppress_message PSM 6
# Reading DEF file
suppress_message ODB 127
# Finished DEF file
suppress_message ODB 134

# suppress ppl info messages. The ones defined in tcl can never
# match between tcl and Python
suppress_message PPL 41
suppress_message PPL 48
suppress_message PPL 49
suppress_message PPL 60

# suppress tap info messages
suppress_message TAP 100
suppress_message TAP 101

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

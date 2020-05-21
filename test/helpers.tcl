# Helper functions common to multiple regressions.

set test_dir [file dirname [file normalize [info script]]]
set result_dir [file join $test_dir "results"]

proc make_result_file { filename } {
  variable result_dir
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
  set diff_line 0;
  while { [gets $stream1 line1] >= 0 && [gets $stream2 line2] >= 0 } {
    if { $line1 != $line2 } {
      set diff_line $line
      break
    }
    incr line
  }
  close $stream1
  close $stream2

  if { $diff_line } {
    puts "Differences found at line $diff_line."
    puts "$line1"
    puts "$line2"
    return 1
  } else {
    puts "No differences found."
    return 0
  }
}

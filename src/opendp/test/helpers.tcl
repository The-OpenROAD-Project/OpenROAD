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
  gets $stream line
  while { ![eof $stream] } {
    puts $line
    gets $stream line
  }
  close $stream
}

proc diff_files { file1 file2 } {
  set stream1 [open $file1 r]
  set stream2 [open $file2 r]
  gets $stream1 line1
  gets $stream2 line2
  set line 1
  set diff_line 0;
  while { ![eof $stream1] && ![eof $stream2] } {
    if { $line1 != $line2 } {
      set diff_line $line
      break
    }
    gets $stream1 line1
    gets $stream2 line2
    incr line
  }
  if { !([eof $stream1] && [eof $stream2]) } {
    set diff_line $line
  }
  close $stream1
  close $stream2
  if { $diff_line } {
    puts "Differences found at line $diff_line."
    return 1
  } else {
    puts "No differences found."
    return 0
  }
}

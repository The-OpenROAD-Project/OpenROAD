# Helper functions common to multiple regressions.

if { [info exists env("RESIZER")] } {
  set result_dir [file join $env("RESIZER") "test" "results"]
} else {
  set test_dir [file dirname [file normalize [info script]]]
  set result_dir [file join $test_dir "results"]
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

proc make_result_file { filename } {
  variable result_dir
  return [file join $result_dir $filename]
}

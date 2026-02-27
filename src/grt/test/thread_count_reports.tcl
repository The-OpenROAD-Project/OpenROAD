source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

proc write_thread_report { output file_name } {
  set stream [open $file_name w]
  set section ""
  set found_wirelength 0
  set found_resources 0
  set found_congestion 0

  foreach line [split $output "\n"] {
    if { $section eq "" } {
      if { [regexp {Routing resources analysis:} $line] } {
        set section "resources"
        set found_resources 1
        puts $stream $line
      } elseif { [regexp {Final congestion report:} $line] } {
        set section "congestion"
        set found_congestion 1
        puts $stream $line
      } elseif { [regexp {Total wirelength:} $line] } {
        set found_wirelength 1
        puts $stream $line
      }
    } else {
      puts $stream $line
      if { $line eq "" } {
        set section ""
      }
    }
  }

  close $stream

  if { !$found_resources || !$found_congestion || !$found_wirelength } {
    utl::error GRT 705 \
      "Failed to capture threaded global-route report output."
  }
}

proc capture_thread_report { thread_count prefix } {
  set_thread_count $thread_count

  set output ""
  tee -variable output -quiet {global_route -verbose}

  set raw_file [make_result_file "${prefix}.raw.rpt"]
  set stream [open $raw_file w]
  puts -nonewline $stream $output
  close $stream

  set report_file [make_result_file "${prefix}.rpt"]
  write_thread_report $output $report_file
  return $report_file
}

set report_file1 [capture_thread_report 1 thread_count_reports1]
set report_file2 [capture_thread_report 2 thread_count_reports2]

if { [diff_files $report_file1 $report_file2] != 0 } {
  exit 1
}

puts "pass"
exit 0

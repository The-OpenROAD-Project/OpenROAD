source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_thread_count 2

set output ""
tee -variable output -quiet {
  global_route \
    -multicore \
    -verbose
}

set report_file [make_result_file "multicore_smoke.rpt"]
set stream [open $report_file w]
puts -nonewline $stream $output
close $stream

set wirelength -1
set vias -1

foreach line [split $output "\n"] {
  if { [regexp {Total wirelength:\s+([0-9]+)\s+um} $line -> value] } {
    set wirelength $value
  } elseif { [regexp {Final number of vias:\s+([0-9]+)} $line -> value] } {
    set vias $value
  }
}

if { $wirelength <= 0 || $vias < 0 } {
  utl::error GRT 706 "Failed to capture multicore global-route summary output."
}

puts "pass"
exit 0

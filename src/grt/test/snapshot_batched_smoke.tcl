source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "shuffle_stress.def"

set_thread_count 16

set_routing_layers -signal metal2-metal6
set_global_routing_layer_adjustment * 0.20
foreach layer {metal2 metal3 metal4 metal5} {
  set_global_routing_region_adjustment {7.8 2.8 10.2 140.0} \
    -layer $layer \
    -adjustment 0.20
}

set output ""
tee -variable output -quiet {
  global_route -allow_congestion -snapshot_batched_width 16 \
    -congestion_iterations 200 -verbose
}

set report_file [make_result_file "snapshot_batched_smoke.rpt"]
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
  utl::error GRT 706 "Failed to capture snapshot-batched global-route summary output."
}

require_snapshot_batched_activity "snapshot_batched_smoke"

puts "pass"
exit 0

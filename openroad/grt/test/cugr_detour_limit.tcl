# An eight-gcell trunk permits two extra gcells of stems (25%). Block the
# straight route and reduce adjacent-row capacity so a cheaper, longer
# detour is tempting. Checking high()-1 incorrectly admits a 12-gcell route.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "cugr_detour_limit.def"
set_routing_layers -signal metal2-metal3

with_output_to_variable route_log {
  global_route -use_cugr -verbose
}
check "Pattern detours are exercised" {
  string match "*Stage 3: Pattern routing with detours.*" $route_log
} 1
check "Maze routing does not mask the detour result" {
  string match "*Stage 4:*" $route_log
} 0

set segments [make_result_file cugr_detour_limit.segs]
write_global_route_segments $segments
set stream [open $segments r]
set wire_length 0
foreach line [split [read $stream] "\n"] {
  if { [llength $line] == 6 } {
    lassign $line x1 y1 layer1 x2 y2 layer2
    incr wire_length [expr { abs($x2 - $x1) + abs($y2 - $y1) }]
  }
}
close $stream
puts "Route length: $wire_length DBU (limit: 42000)"
check "Route detours around the blockage" { expr { $wire_length > 33600 } } 1
check "Detour adds at most 25% to the 33600 DBU trunk" {
  expr { $wire_length <= 42000 }
} 1
exit_summary

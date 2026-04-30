set_thread_count 16

rename global_route global_route_serial
proc global_route { args } {
  uplevel 1 [list global_route_serial {*}$args -snapshot_batched_width 16]
}

source "bus_route.tcl"

puts "pass"
exit 0

set_thread_count 16

rename global_route global_route_serial
proc global_route {args} {
  uplevel 1 [list global_route_serial {*}$args -multicore]
}

source "bus_route.tcl"

puts "pass"
exit 0


proc detailed_route_server { args } {
  sta::check_argc_eq1 "detailed_route_server" $args
  dst::run_server_cmd $args
}

proc run_load_balancer { args } {
  sta::check_argc_eq1 "run_load_balancer" $args
  dst::run_load_balancer $args
}

proc add_worker_address { args } {
  sta::check_argc_eq2 "add_worker_address" $args
  dst::add_worker_address [lindex $args 0] [lindex $args 1]
}

sta::define_cmd_args "detailed_route_server" {
    [-port port]
    [-shared_volume vol]
}
proc detailed_route_server { args } {
  sta::parse_key_args "detailed_route_server" args \
    keys {-port -shared_volume} \
    flags {}
  sta::check_argc_eq0 "detailed_route_server" $args
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  } else {
    utl::error DST 20 "-port is required."
  }
  if { [info exists keys(-shared_volume)] } {
    set shared_volume $keys(-shared_volume)
  } else {
    utl::error DST 21 "-shared_volume is required."
  }
  dst::run_server_cmd $port $shared_volume
}

proc run_load_balancer { args } {
  sta::check_argc_eq1 "run_load_balancer" $args
  dst::run_load_balancer $args
}

proc add_worker_address { args } {
  sta::check_argc_eq2 "add_worker_address" $args
  dst::add_worker_address [lindex $args 0] [lindex $args 1]
}

sta::define_cmd_args "run_worker" {
    [-port port]
}
proc run_worker { args } {
  sta::parse_key_args "run_worker" args \
    keys {-port} \
    flags {}
  sta::check_argc_eq0 "run_worker" $args
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  } else {
    utl::error DST 10 "-port is required."
  }
  dst::run_server_cmd $port
}

proc run_load_balancer { args } {
  sta::check_argc_eq1 "run_load_balancer" $args
  dst::run_load_balancer $args
}

proc add_worker_address { args } {
  sta::check_argc_eq2 "add_worker_address" $args
  dst::add_worker_address [lindex $args 0] [lindex $args 1]
}
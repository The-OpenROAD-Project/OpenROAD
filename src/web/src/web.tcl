# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

sta::define_cmd_args "web_server" { [-host host] [-port port] -dir dir }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {-host -port -dir} flags {}

  sta::check_argc_eq0 "web_server" $args

  set host "127.0.0.1"
  if { [info exists keys(-host)] } {
    set host $keys(-host)
  }

  set port 8080
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  }

  if { ![info exists keys(-dir)] } {
    utl::error WEB 19 "-dir is required: pass the path to the web assets directory."
  }

  web::web_server_cmd $host $port $keys(-dir)
}

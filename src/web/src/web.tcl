# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

sta::define_cmd_args "web_server" { [-dir dir] [-port port] }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {-dir -port} flags {}

  sta::check_argc_eq0 "web_server" $args

  set port 8080
  set doc_root ""
  if { [info exists keys(-dir)] } {
    set doc_root $keys(-dir)
  }
  if { [info exists keys(-port)] } {
    set p $keys(-port)
    if { ![string is integer -strict $p] || $p < 1 || $p > 65535 } {
      error "Invalid port number: $p. Must be an integer between 1 and 65535."
    }
    set port $p
  }

  web::web_server_cmd $port $doc_root
}

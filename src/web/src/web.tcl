# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

sta::define_cmd_args "web_server" { [-dir dir] }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {-dir} flags {}

  sta::check_argc_eq0 "web_server" $args

  set doc_root ""
  if { [info exists keys(-dir)] } {
    set doc_root $keys(-dir)
  }

  web::web_server_cmd $doc_root
}

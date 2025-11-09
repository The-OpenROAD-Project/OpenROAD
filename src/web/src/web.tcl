# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

sta::define_cmd_args "web_server" { }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {} flags {}

  sta::check_argc_eq0 "web_server" $args

  web::web_server_cmd
}

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

sta::define_cmd_args "web_server" { [-port port] -dir dir }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {-port -dir} flags {}

  sta::check_argc_eq0 "web_server" $args

  set port 8080
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  }

  if { ![info exists keys(-dir)] } {
    utl::error WEB 19 "-dir is required: pass the path to the web assets directory."
  }

  web::web_server_cmd $port $keys(-dir)
}

sta::define_cmd_args "web_export_json" { -output output_file \
    [-design design_name] [-stage stage_name] [-variant variant_name] }

proc web_export_json { args } {
  sta::parse_key_args "web_export_json" args \
    keys {-output -design -stage -variant} flags {}

  sta::check_argc_eq0 "web_export_json" $args

  if { ![info exists keys(-output)] } {
    utl::error WEB 20 "-output is required."
  }

  set design ""
  if { [info exists keys(-design)] } {
    set design $keys(-design)
  }
  set stage ""
  if { [info exists keys(-stage)] } {
    set stage $keys(-stage)
  }
  set variant ""
  if { [info exists keys(-variant)] } {
    set variant $keys(-variant)
  }

  web::web_export_json_cmd $keys(-output) $design $stage $variant
}

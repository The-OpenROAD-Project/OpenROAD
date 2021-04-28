record_tests {
  import_package
  read_lef
  read_zipped
  create_sboxes
  dump_via_rules
  dump_vias
  read_def
  dump_nets
  lef_mask
  write_lef_and_def
  lef_data_access
  gcd_def_access
  gcd_pdn_def_access
  edit_def
  wire_encoder
  edit_via_params
  row_settings
  db_read_write
  check_routing_tracks
  polygon
  def_parser
  ndr
}

record_pass_fail_tests {
  cpp_tests
  dump_netlists
  dump_netlists_withfill
}


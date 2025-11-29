# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# Example user-level command with argument processing.

sta::define_cmd_args "cgv_display_csv" { \
                           [-csv file_name]}

proc cgv_display_csv { args } {
  sta::parse_key_args "cgv_display_csv" args \
    keys {-csv} flags {}


  sta::check_argc_eq0 "cgv_display_csv" $args

  cgv::display_csv_cmd  $keys(-csv)
}
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

sta::define_cmd_args "check_antennas" { [-verbose] \
                                        [-report_violating_nets] \
                                        [-report_file report_file] \
                                        [-net net]
} ;# checker off

proc check_antennas { args } {
  sta::parse_key_args "check_antennas" args \
    keys {-report_file -net} \
    flags {-verbose -report_violating_nets} ;# checker off

  if { [ord::get_db_block] == "NULL" } {
    utl::error ANT 3 "No design block found."
  }

  sta::check_argc_eq0 "check_antennas" $args

  # set report_file ""
  if { [info exists keys(-report_file)] } {
    set report_file $keys(-report_file)
    ant::set_report_file_name $report_file
  }

  set verbose [info exists flags(-verbose)]

  set net_name ""
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  }

  if { [info exists keys(-report_filename)] } {
    utl::warn ANT 10 "-report_filename is deprecated."
  }
  if { [info exists flags(-report_violating_nets)] } {
    utl::warn ANT 11 "-report_violating_nets is deprecated."
  }
  return [ant::check_antennas $net_name $verbose]
}

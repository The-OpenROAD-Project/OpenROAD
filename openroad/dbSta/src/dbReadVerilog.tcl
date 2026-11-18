# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# Read Verilog to OpenDB

sta::define_cmd_args "read_verilog" {filename}

proc read_verilog { filename } {
  ord::read_verilog_cmd [file nativename $filename]
}

sta::define_cmd_args "link_design" {[-hier] [-omit_filename_prop] top_cell_name}

proc link_design { args } {
  sta::parse_key_args "link_design" args keys {} \
    flags {-hier -omit_filename_prop}

  set hierarchy [info exists flags(-hier)]
  set omit_filename_prop [info exists flags(-omit_filename_prop)]
  sta::check_argc_eq1 "link_design" $args
  set top_cell_name [lindex $args 0]

  if { ![ord::db_has_tech] } {
    utl::error ORD 2010 "no technology has been read."
  }
  ord::link_design_db_cmd $top_cell_name $hierarchy $omit_filename_prop
}

sta::define_cmd_args "write_verilog" {[-sort] [-include_pwr_gnd]\
					  [-remove_cells cells] filename}

# Copied from sta/verilog/Verilog.tcl because we don't want sta::read_verilog
# that is in the same file.
proc write_verilog { args } {
  sta::parse_key_args "write_verilog" args keys {-remove_cells} \
    flags {-sort -include_pwr_gnd}
  if { [info exists flags(-sort)] } {
    utl::warn STA 2065 "The -sort flag is ignored."
  }
  set remove_cells {}
  if { [info exists keys(-remove_cells)] } {
    set remove_cells [sta::parse_cell_arg $keys(-remove_cells)]
  }
  set include_pwr_gnd [info exists flags(-include_pwr_gnd)]
  sta::check_argc_eq1 "write_verilog" $args
  set filename [file nativename [lindex $args 0]]
  sta::write_verilog_cmd $filename $include_pwr_gnd $remove_cells
}

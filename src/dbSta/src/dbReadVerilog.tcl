# Read Verilog to OpenDB
# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

sta::define_cmd_args "read_verilog" {filename}

proc read_verilog { filename } {
  ord::read_verilog_cmd [file nativename $filename]
}

sta::define_cmd_args "link_design" {[top_cell_name]}

proc link_design { {top_cell_name ""} } {
  variable current_design_name

  if { $top_cell_name == "" } {
    if { $current_design_name == "" } {
      sta::sta_error "missing top_cell_name argument and no current_design."
      return 0
    } else {
      set top_cell_name $current_design_name
    }
  }
  if { ![ord::db_has_tech] } {
    sta::sta_error "no technology has been read."
  }
  ord::link_design_db_cmd $top_cell_name
}

sta::define_cmd_args "write_verilog" {[-sort] filename}

proc write_verilog { args } {
  sta::parse_key_args "write_verilog" args keys {} flags {-sort}

  set sort [info exists flags(-sort)]
  sta::check_argc_eq1 "write_verilog" $args
  set filename [file nativename $args]
  ord::write_verilog_cmd $filename $sort
}

# Verilog2db, Verilog to OpenDB
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

namespace eval sta {

proc show_splash {} {
  puts "OpenROAD [openroad_version] [string range [openroad_git_sha1] 0 9]
License GPLv3: GNU GPL version 3 <http://gnu.org/licenses/gpl.html>

This is free software, and you are free to change and redistribute it
under certain conditions; type `show_copying' for details. 
This program comes with ABSOLUTELY NO WARRANTY; for details type `show_warranty'."
}

# Defined by swig.
define_cmd_args "init_sta_db" {}

# -library is the default
define_cmd_args "read_lef" {[-tech] [-library] filename}

proc read_lef { args } {
  parse_key_args "read_lef" args keys {} flags {-tech -library}
  check_argc_eq1 "read_lef" $args
  set filename $args
  set make_tech [info exists flags(-tech)]
  set make_lib [info exists flags(-library)]
  if { !$make_tech && !$make_lib} {
    set make_lib 1
    set make_tech [expr ![sta::db_has_tech]]
  }
  set lib_name [file rootname [file tail $filename]]
  read_lef_cmd $filename $lib_name $make_tech $make_lib
}

define_cmd_args "read_def" {filename}

proc read_def { args } {
  check_argc_eq1 "read_def" $args
  set filename $args
  read_def_cmd $filename
}

define_cmd_args "write_def" {filename}

proc write_def { args } {
  check_argc_eq1 "write_def" $args
  set filename $args
  write_def_cmd $filename
}

define_cmd_args "read_db" {filename}

proc read_db { args } {
  check_argc_eq1 "read_db" $args
  set filename $args
  read_db_cmd $filename
}

define_cmd_args "write_db" {filename}

proc write_db { args } {
  check_argc_eq1 "write_db" $args
  set filename $args
  write_db_cmd $filename
}

# sta namespace end
}

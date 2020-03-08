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

proc show_openroad_splash {} {
  puts "OpenROAD [ord::openroad_version] [string range [ord::openroad_git_sha1] 0 9]
License GPLv3: GNU GPL version 3 <http://gnu.org/licenses/gpl.html>

This is free software, and you are free to change and redistribute it
under certain conditions; type `show_copying' for details. 
This program comes with ABSOLUTELY NO WARRANTY; for details type `show_warranty'."
}

# -library is the default
sta::define_cmd_args "read_lef" {[-tech] [-library] filename}

proc read_lef { args } {
  sta::parse_key_args "read_lef" args keys {} flags {-tech -library}
  sta::check_argc_eq1 "read_lef" $args

  set filename [file nativename $args]
  if { ![file exists $filename] } {
    sta::sta_error "$filename does not exist."
  }
  if { ![file readable $filename] } {
    sta::sta_error "$filename is not readable."
  }

  set make_tech [info exists flags(-tech)]
  set make_lib [info exists flags(-library)]
  if { !$make_tech && !$make_lib} {
    set make_lib 1
    set make_tech [expr ![ord::db_has_tech]]
  }
  set lib_name [file rootname [file tail $filename]]
  ord::read_lef_cmd $filename $lib_name $make_tech $make_lib
}

sta::define_cmd_args "read_def" {[-order_wires] filename}

proc read_def { args } {
  sta::parse_key_args "read_def" args keys {} flags {-order_wires}
  sta::check_argc_eq1 "read_def" $args
  set filename [file nativename $args]
  if { ![file exists $filename] } {
    sta::sta_error "$filename does not exist."
  }
  if { ![file readable $filename] } {
    sta::sta_error "$filename is not readable."
  }
  if { ![ord::db_has_tech] } {
    sta::sta_error "no technology has been read."
  }
  set order_wires [info exists flags(-order_wires)]
  ord::read_def_cmd $filename $order_wires
}

sta::define_cmd_args "write_def" {[-version version] filename}

proc write_def { args } {
  sta::parse_key_args "write_def" args keys {-version} flags {}

  set version "5.8"
  if { [info exists keys(-version)] } {
    set version $keys(-version)
    if { !($version == "5.8" \
	     || $version == "5.6" \
	     || $version == "5.5" \
	     || $version == "5.4" \
	     || $version == "5.3") } {
      sta::sta_error "DEF versions 5.8, 5.6, 5.4, 5.3 supported."
    }
  }

  sta::check_argc_eq1 "write_def" $args
  set filename [file nativename $args]
  ord::write_def_cmd $filename $version
}

sta::define_cmd_args "read_db" {filename}

proc read_db { args } {
  sta::check_argc_eq1 "read_db" $args
  set filename [file nativename $args]
  if { ![file exists $filename] } {
    sta::sta_error "$filename does not exist."
  }
  if { ![file readable $filename] } {
    sta::sta_error "$filename is not readable."
  }
  ord::read_db_cmd $filename
}

sta::define_cmd_args "write_db" {filename}

proc write_db { args } {
  sta::check_argc_eq1 "write_db" $args
  set filename $args
  ord::write_db_cmd $filename
}

################################################################

namespace eval ord {

trace variable ::file_continue_on_error "w" \
  ord::trace_file_continue_on_error

# Sync with sta::sta_continue_on_error used by 'source' proc defined by OpenSTA.
proc trace_file_continue_on_error { name1 name2 op } {
  set ::sta_continue_on_error $::file_continue_on_error
}

proc error { what } {
  ::error "Error: $what"
}

proc warn { what } {
  puts "Warning: $what"
}

proc ensure_units_initialized { } {
  if { ![units_initialized] } {
    sta::sta_error "Command units uninitialized. Use the read_liberty or set_cmd_units command to set units."
  }
}

# namespace ord
}

# redefine sta::sta_error to call ord::error
namespace eval sta {

proc sta_error { msg } {
  ord::error $msg
}

# namespace sta
}

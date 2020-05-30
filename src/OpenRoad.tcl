############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################

proc show_openroad_splash {} {
  puts "OpenROAD [ord::openroad_version] [string range [ord::openroad_git_sha1] 0 9]
This program is licensed under the BSD-3 license. See the LICENSE file for details. 
Components of the program may be licensed under more restrictive licenses which must be honored."
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

sta::define_cmd_args "read_def" {[-order_wires] [-continue_on_errors] filename}

proc read_def { args } {
  sta::parse_key_args "read_def" args keys {} flags {-order_wires -continue_on_errors}
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
  set continue_on_errors [info exists flags(-continue_on_errors)]
  ord::read_def_cmd $filename $order_wires $continue_on_errors
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

proc clear {} {
  [get_db] clear
  sta::clear_network
  sta::clear_sta
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

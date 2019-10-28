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

# Temporarily stolen from sta/Sdc.tcl
set ::sta_continue_on_error 1

define_cmd_args "source" \
  {[-echo] [-verbose] filename [> filename] [>> filename]}

# Override source to support -echo and return codes.
proc_redirect source {
  parse_key_args "source" args keys {-encoding} flags {-echo -verbose}
  if { [llength $args] != 1 } {
    cmd_usage_error "source"
  }
  set echo [info exists flags(-echo)]
  set verbose [info exists flags(-verbose)]
  set filename [lindex $args 0]
  source_ $filename $echo $verbose
}

proc source_ { filename echo verbose } {
  global sta_continue_on_error
  variable sdc_file
  variable sdc_line
  if [catch {open $filename r} stream] {
    sta_error "cannot open '$filename'."
  } else {
    # Save file and line in recursive call to source.
    if { [info exists sdc_file] } {
      set sdc_file_save $sdc_file
      set sdc_line_save $sdc_line
    }
    set sdc_file $filename
    set sdc_line 1
    set cmd ""
    set errors 0
    while {![eof $stream]} {
      gets $stream line
      if { $line != "" } {
	if {$echo} {
	  puts $line
	}
      }
      append cmd $line "\n"
      if { [string index $line end] != "\\" \
	     && [info complete $cmd] } {
	set error {}
	switch [catch {uplevel \#0 $cmd} result] {
	  0 { if { $verbose && $result != "" } { puts $result } }
	  1 { set error $result }
	  2 { set error {invoked "return" outside of a proc.} }
	  3 { set error {invoked "break" outside of a loop.} }
	  4 { set error {invoked "continue" outside of a loop.} }
	}
	set cmd ""
	if { $error != {} } {
	  if { [string first "Error" $error] == 0 } {
	    puts $error
	  } else {
	    puts "Error: [file tail $sdc_file], $sdc_line $error"
	  }
	  set errors 1
	  if { !$sta_continue_on_error } {
	    break
	  }
	}
      }
      incr sdc_line
    }
    close $stream
    if { $cmd != {} } {
      sta_error "incomplete command at end of file."
    }
    if { [info exists sdc_file_save] } {
      set sdc_file $sdc_file_save
      set sdc_line $sdc_line_save
    } else {
      unset sdc_file
      unset sdc_line
    }
    return $errors
  }
}

################################################################

proc show_splash {} {
  puts "OpenStaDB [sta::opensta_db_version] [string range [sta::opensta_db_git_sha1] 0 9] Copyright (c) 2019, Parallax Software, Inc.
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

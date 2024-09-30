###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2024, The Regents of the University of California
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
##   and#or other materials provided with the distribution.
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
###############################################################################

# This code contains the Tcl-native implementation of man command.

global MAN_PATH
set MAN_PATH ""

sta::define_cmd_args "man" { name\
                            [-manpath manpath]\
                            [-no_pager]}
proc man { args } {
  sta::parse_key_args "man" args \
    keys {-manpath} flags {-no_pager}

  set name [lindex $args 0]

  # check the default man path based on executable path
  set exec_output [info nameofexecutable]

  # Check if the output contains 'build/src'
  if { [string match "*build/src*" $exec_output] } {
    set executable_path [file normalize [file dirname [info nameofexecutable]]]
    set man_path [file normalize [file dirname [file dirname $executable_path]]]
    set DEFAULT_MAN_PATH [file join $man_path "docs" "cat"]
  } else {
    set DEFAULT_MAN_PATH "/usr/local/share/man/cat"
  }

  global MAN_PATH
  if { [info exists keys(-manpath)] } {
    set MAN_PATH $keys(-manpath)
    if { [utl::check_valid_man_path $MAN_PATH] == false } {
      puts "Using default manpath."
      set MAN_PATH $DEFAULT_MAN_PATH
    }
  } else {
    set MAN_PATH $DEFAULT_MAN_PATH
  }

  set no_pager 0
  if { [info exists flags(-no_pager)] } {
    set no_pager 1
  }

  #set MAN_PATH [utl::get_input]
  #if { [utl::check_valid_man_path $MAN_PATH] == false } {
  #  puts "Using default manpath."
  #  set MAN_PATH $DEFAULT_MAN_PATH
  #}

  set man_path $MAN_PATH
  set man_sections {}
  foreach man_section {cat1 cat2 cat3} {
    lappend man_sections "$man_path/$man_section"
  }
  set man_found false
  foreach man_section $man_sections {
    set length [string length $man_section]
    # Get suffix for man section
    set man_suffix [string range $man_section [expr { $length - 1 }] $length]
    # Replace all "::" with "_"
    set name1 [string map { "::" "_" } $name]
    append name1 ".$man_suffix"
    set man_file [file join $man_section $name1]
    if { [file exists $man_file] } {
      set man_found true
      set file_handle [open $man_file r]
      set content [read $file_handle]
      close $file_handle

      # Display content
      set lines [split $content "\n"]
      set num_lines [llength $lines]
      set page_size 40

      for { set i 0 } { $i < $num_lines } { incr i $page_size } {
        set page [lrange $lines $i [expr { $i + $page_size - 1 }]]
        puts [join $page "\n"]

        # Ask user to continue or quit
        if { !$no_pager && [llength $lines] > $page_size } {
          puts -nonewline "---\nPress 'q' to quit or any other key to continue: \n---"
          flush stdout
          set input [gets stdin]
          if { $input == "q" } {
            break
          }
        }
      }
    }
  }

  if { $man_found == false } {
    utl::error UTL 100 "Man page for $name is not found."
  }
}

namespace eval utl {
proc get_input { } {
  # Get the relative path from the user
  puts "Please enter an optional relative path to the cat folders:"
  flush stdout
  gets stdin relative_path

  # Convert the relative path to an absolute path
  set absolute_path [file join [pwd] $relative_path]

  # Display the absolute path
  puts "Absolute Path: $absolute_path"
  if { [utl::check_valid_path $absolute_path] } {
    return $absolute_path
  }
  return ""
}

proc check_valid_path { path } {
  if { [file isdirectory $path] } {
    return true
  } else {
    puts "Invalid path, please retry."
    return false
  }
}

proc check_valid_man_path { path } {
  if { [file isdirectory "$path/cat1"] } {
    return true
  } else {
    puts "Invalid man path, please retry."
    return false
  }
}

# utl namespace end
}
